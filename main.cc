#include <chrono>
#include <cstdint>
#include <ctime>
#include <gtkmm.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <string>

Gtk::Label* clock_label = nullptr;
Gtk::Label* diff_label = nullptr;
Gtk::Label* status_label = nullptr;
Gtk::TextView* log_textview = nullptr;

// TODO: This should be done with styles probably
Pango::AttrColor green_attr = Pango::Attribute::create_attr_foreground(0x8a8a, 0xe2e2, 0x3434);
Pango::AttrColor red_attr = Pango::Attribute::create_attr_foreground(0xefef, 0x2929, 0x2929);

void fill_in_time();
bool clock_update();

char* time_string = new char[256];
char* diff_string = new char[256];

// Abstract away getting the time since the epoch
// TODO: is this use of std::chrono well defined?
int64_t time_since_epoch() {
	using std::chrono::system_clock;
	const auto now = system_clock::now();
	const auto epoch = now.time_since_epoch();
	const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(epoch);

	return seconds.count();
}

// XXX: These are all hacks to avoid properly os-dependant time gathering calls
// we just keep 2 copies of the times in the 2 different formats
time_t in_time_c = -1;

int64_t in_time = -1;
int64_t out_time = -1;
bool punched_in = false;

std::ofstream* log_output = nullptr;

// TODO: We could probably do better by natively interacting with the textview's buffer
// this is just used for easy conversion
std::stringstream log_sstream;

// TODO: This should use exceptions to report errors
// TODO: use an enum to represent the stamp to use
// TODO: This should update the log too
// TODO: Switch to a format where each line represents an event rather than a session
void stamp_file(bool stamp_in) {
	fill_in_time();
	if (stamp_in) {
		*log_output << in_time;
		log_sstream << time_string;
	} else {
		*log_output << '\t' << out_time << std::endl;
		log_sstream << '\t' << time_string << std::endl;
	}
	log_textview->get_buffer()->set_text(Glib::ustring(log_sstream.str()));
}

void punch_in() {
	if (!punched_in) {
		std::cout << "Punched in: " << time_string << std::endl;
		punched_in = true;
		in_time = time_since_epoch();
		in_time_c = time(0);

		stamp_file(true);

		Pango::AttrList attribs = status_label->get_attributes();
		attribs.change(green_attr);
		status_label->set_text(Glib::ustring("IN"));
		status_label->set_attributes(attribs);
		
		// XXX: This is hacky, but allows the diff label to appear right away
		clock_update();
	} else {
		std::cout << "Already punched in" << std::endl;
	}
}

void punch_out() {
	if (punched_in) {
		std::cout << "Punched out: " << time_string << std::endl;
		punched_in = false;
		out_time = time_since_epoch();

		stamp_file(false);
		
		Pango::AttrList attribs = status_label->get_attributes();
		attribs.change(red_attr);
		status_label->set_text(Glib::ustring("OUT"));
		status_label->set_attributes(attribs);

		// XXX: This is hacky, but allows the diff label to disappear right away
		clock_update();
	} else {
		std::cout << "Not punched in" << std::endl;
	}
}

// This abstracts the time_string update from the clock signal callback
// allowing the timestring to be pre-initialized, since it is used in
// other places, thus preventing data races.
void fill_in_time() {
	time_t rawtime;
	tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(time_string, 256, "%F %r", timeinfo);

	if (punched_in) {
		// Handle updating the diff_string when punched in
		time_t timer;
		time(&timer);
		int64_t seconds = difftime(timer,in_time_c);
		int seconds_place = seconds % 60;
		int minutes_place = seconds / 60;
		int hours_place = seconds / (3600);
		snprintf(diff_string, 256, "%02d hours, %02d minutes and %02d seconds", hours_place, \
				minutes_place, seconds_place);
	}
}

// Handles updating both the wallclock and the wallclock diff
bool clock_update() {
	fill_in_time();
	clock_label->set_text(Glib::ustring(time_string));
	if (punched_in) {
		diff_label->set_text(Glib::ustring(diff_string));
	} else {
		diff_label->set_text(Glib::ustring("N/A"));
	}

	return true;
}

int main(int argc, char *argv[]) {
	// TODO: Don't hardcode this
	std::string filename = "timesheet.tsv";

	// TODO: This should be smarter about append/overwrite
	if (!log_output) {
		log_output = new std::ofstream(filename.c_str(), std::ofstream::app);
		// Always append a line in case the punch out didn't happen
		// TODO: Cleanup with a punch out/special unpunched time record
		*log_output << std::endl;
	}

	auto app = Gtk::Application::create(argc, argv, "org.gtkmm.example");

	auto refBuilder = Gtk::Builder::create();
	try {
		refBuilder->add_from_file("tracking.glade");
	} catch (const Glib::FileError& ex) {
		std::cerr << "FileError: " << ex.what() << std::endl;
		return 1;
	} catch (const Glib::MarkupError& ex) {
		std::cerr << "MarkupError: " << ex.what() << std::endl;
		return 1;
	} catch (const Gtk::BuilderError& ex) {
		std::cerr << "BuilderError: " << ex.what() << std::endl;
		return 1;
	}

	Gtk::ApplicationWindow* window = nullptr;
	refBuilder->get_widget("main_window", window);
	if (window) {
		// TODO: Is there a better place for this?
		fill_in_time();

		// Both are required by clock_update()
		refBuilder->get_widget("worked_time", diff_label);
		refBuilder->get_widget("current_time", clock_label);
		if (clock_label && diff_label) {
			// Using connect_second at interval of 1 might save battery, but will lead
			// to an inaccurate clockface
			Glib::signal_timeout().connect(sigc::ptr_fun(&clock_update), 500);
			// Run an update right away to have it be accurate when the app starts
			clock_update();
		}

		refBuilder->get_widget("status", status_label);
		refBuilder->get_widget("timestamp_log", log_textview);

		Gtk::Button* punch_in_button = nullptr;
		refBuilder->get_widget("in", punch_in_button);
		if (punch_in_button) {
			punch_in_button->signal_clicked().connect(sigc::ptr_fun(&punch_in));
		}
		
		Gtk::Button* punch_out_button = nullptr;
		refBuilder->get_widget("out", punch_out_button);
		if (punch_out_button) {
			punch_out_button->signal_clicked().connect(sigc::ptr_fun(&punch_out));
		}

		app->run(*window);
	}

	log_output->close();

	delete window;
	delete log_output;
	delete[] time_string;
	delete[] diff_string;

	return 0;
}

