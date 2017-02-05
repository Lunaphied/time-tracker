#include <ctime>
#include <gtkmm.h>
#include <iostream>

Gtk::Label* clock_label = nullptr;
char* time_string = new char[128];

void punch_in() {
	std::cout << "Punched in" << std::endl;
}

void punch_out() {
	std::cout << "Punched out" << std::endl;
}

bool clock_update() {
	std::time_t rawtime;
	std::tm* timeinfo;
	std::time(&rawtime);
	timeinfo = std::localtime(&rawtime);

	strftime(time_string, 128, "%r", timeinfo);

	clock_label->set_text(Glib::ustring(time_string));
	return true;
}

int main(int argc, char *argv[]) {
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
		refBuilder->get_widget("current_time", clock_label);
		if (clock_label) {
			Glib::signal_timeout().connect_seconds(sigc::ptr_fun(&clock_update), 1);
		}

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

	delete window;

	return 0;
}

