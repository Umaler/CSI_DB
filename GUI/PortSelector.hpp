#pragma once

#include <gtkmm.h>

class PortSelector : public Gtk::Window {
public:

    PortSelector(Gtk::Window& parent);

    unsigned int getValue() const;
    sigc::signal<void(Gtk::ResponseType)> signalOnResponse() const;

private:
    Gtk::Grid grid;

    Glib::RefPtr<Gtk::Adjustment> buttonAdj;
    Gtk::SpinButton button;
    Gtk::Button okButton{"Ok"}, cancelButton{"Cancel"};

    sigc::signal<void(Gtk::ResponseType)> onResponse;

};
