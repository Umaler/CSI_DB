#pragma once

#include <gtkmm.h>

#include <string>
#include "DataSourcePlotWindow.hpp"

namespace WMG {

class MainWindow : public Gtk::ApplicationWindow {
public:
    MainWindow();

private:

    void onOpenDSPWindow();
    void onOpenRTDSWindow();
    void onOpenDBPDWindow();

    const std::string title = "DB plotter";
    const unsigned int width = 400, height = 400;

    Gtk::Box mainBox;

    Gtk::Button openDSPButton{"Open Data Source Plot Window"};
    Gtk::Button openRTDSButton{"Open Real Time DS Window"};

    std::unique_ptr<DataSourcePlotWindow> dspWindow;
    std::unique_ptr<DataSourcePlotWindow> rtdsWindow;
    std::unique_ptr<DataSourcePlotWindow> dbpdWindow;

};

}
