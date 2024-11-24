#include "MainWindow.hpp"

#include <iostream>

#include "../DataSources/DBSource.hpp"
#include "../DataSources/RTSource.hpp"
#include "../DataSources/DBPhaseDiffSource.hpp"

#include <SFML/Network.hpp>

#include "PortSelector.hpp"

namespace WMG {

MainWindow::MainWindow() :
    mainBox(Gtk::Orientation::VERTICAL)
{
    set_title(title);
    set_default_size(width, height);

    set_child(mainBox);

    mainBox.set_margin(5);
    mainBox.set_spacing(5);
    mainBox.set_expand();

    mainBox.append(openDSPButton);
    openDSPButton.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::onOpenDSPWindow) );

    mainBox.append(openRTDSButton);
    openRTDSButton.signal_clicked().connect( sigc::mem_fun(*this, &MainWindow::onOpenRTDSWindow) );
}

void MainWindow::onOpenDSPWindow() {
    if(dspWindow)
        return;

    Glib::RefPtr<Gtk::FileChooserNative> dialog = Gtk::FileChooserNative::create("Choose database", *this, Gtk::FileChooser::Action::OPEN);
    dialog->set_modal();

    dialog->signal_response().connect(
        [dialog, this](int response_id) {
            if(response_id != Gtk::ResponseType::ACCEPT) {
                return;
            }

            try {
                SQLite::Database db(dialog->get_file()->get_path());

                dspWindow.reset(new DataSourcePlotWindow(std::make_unique<DBSource>(std::move(db))));
                dspWindow->signal_unmap().connect( [&](){dspWindow.reset();} );
                dspWindow->set_title("Data base plot");
                dspWindow->show();
            }
            catch(std::exception& ex) {
                std::cerr << ex.what();
            }
        }
    );

    dialog->show();
}

void MainWindow::onOpenRTDSWindow() {
    if(rtdsWindow)
        return;

    std::shared_ptr<PortSelector> selector(new PortSelector(*this));
    selector->signalOnResponse().connect([selector, this](Gtk::ResponseType response) {
            if(response != Gtk::ResponseType::OK)
                return;

            if (sf::UdpSocket socket; socket.bind(selector->getValue()) != sf::Socket::Status::Done) {
                auto ad = Gtk::AlertDialog::create("Unable to use this port");
                ad->show();
                return;
            }

            Glib::RefPtr<Gtk::FileChooserNative> dialog = Gtk::FileChooserNative::create("Choose database", *this, Gtk::FileChooser::Action::OPEN);
            dialog->set_modal();

            dialog->signal_response().connect(
                [selector, dialog, this](int response_id) {
                    if(response_id != Gtk::ResponseType::ACCEPT) {
                        return;
                    }

                    try {
                        rtdsWindow.reset(new DataSourcePlotWindow(std::make_unique<RTSource>(dialog->get_file()->get_path(), selector->getValue())));
                        rtdsWindow->signal_unmap().connect( [&](){rtdsWindow.reset();} );
                        rtdsWindow->set_title("Real time plot");
                        rtdsWindow->show();
                    }
                    catch(std::exception& ex) {
                        std::cerr << ex.what();
                    }
                }
            );

            dialog->show();
        }
    );

    selector->show();
}

}

