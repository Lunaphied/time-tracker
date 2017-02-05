#include <chrono>
#include <cstdint>
#include <ctime>
#include <gtkmm.h>
#include <fstream>
#include <iostream>
#include <string>

Gtk::Label* clock_label = nullptr;
Gtk::Label* status_label = nullptr;

// TODO: This should be done with styles probably
Pango::AttrColor green_attr = Pango::Attribute::create_attr_foreground(0x8a8a, 0xe2e2, 0x3434);
Pango::AttrColor red_attr = Pango::Attribute::create_attr_foreground(0xefef, 0x2929, 0x2929);

char* time_string = new char[128];

// Abstract away getting the time since the epoch
// TODO: is this use of std::chrono well defined?
int64_t time_since_epoch() {
	using std::chrono::system_clock;
	const auto now = system_clock::now();
	const auto epoch = now.time_since_epoch();
	const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(epoch);

	return seconds.count();
}

int64_t in_time = -1;
int64_t out_time = -1;
bool punched_in = false;

std::ofstream* log_output = nullptr;

// TODO: This should use exceptions to report errors
// TODO: use an enum to represent the stamp to use
// TODO: This should update the log too
void stamp_file(bool stamp_in) {
	if (stamp_in) {
		*log_output << in_time;
	} else {
		*log_output << '\t' << out_time << std::endl;
	}
}

void punch_in() {
	if (!punched_in) {
		std::cout << "Punched in: " << time_string << std::endl;
		punched_in = true;
		in_time = time_since_epoch();
		stamp_file(true);

		Pango::AttrList attribs = status_label->get_attributes();
		attribs.change(green_attr);
		status_label->set_text(Glib::ustring("IN"));
		status_label->set_attributes(attribs);
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

	strftime(time_string, 128, "%r", timeinfo);
}

bool clock_update() {
	fill_in_time();
	clock_label->set_text(Glib::ustring(time_string));
	return true;
}

int main(int argc, char *argv[]) {
	// TODO: Don't hardcode this
	std::string filename = "timesheet.tsv";

	// TODO: This should be smarter about append/overwrite
	if (!log_output) {
		log_output = new std::ofstream(filename.c_str());
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

		refBuilder->get_widget("current_time", clock_label);
		if (clock_label) {
			// Using connect_second at interval of 1 might save battery, but will lead
			// to an inaccurate clockface
			Glib::signal_timeout().connect(sigc::ptr_fun(&clock_update), 500);
			// Run an update right away to have it be accurate when the app starts
			clock_update();
		}

		refBuilder->get_widget("status", status_label);

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
	delete time_string;

	return 0;
}

