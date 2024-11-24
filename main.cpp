#include <iostream>
#include <string>
#include <cstdlib>

#include <gtkmm.h>

#include "GUI/MainWindow.hpp"

#include "DataSources/DBSource.hpp"

int main(int argc, char *argv[])
{
    #ifdef __linux__
        // because since gtk 4.14 default behavior is
        // to prefer gles over gl
        // gitlab.gnome.org/GNOME/gtk/-/issues/6589
        std::string var("GDK_DEBUG=gl-prefer-gl");
        putenv(&var[0]);
    #endif

    auto app = Gtk::Application::create();

    return app->make_window_and_run<WMG::MainWindow>(argc, argv);
}
