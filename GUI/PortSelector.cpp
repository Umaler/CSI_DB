#include "PortSelector.hpp"

PortSelector::PortSelector(Gtk::Window& parent) :
    buttonAdj(Gtk::Adjustment::create(0, 0, 65535, 1, 1)),
    button(buttonAdj)
{
    set_modal(true);
    set_transient_for(parent);
    set_title("Set port");
    set_size_request(200, 100);

    okButton.signal_clicked().connect([&]() {
            onResponse.emit(Gtk::ResponseType::OK);
            hide();
        }
    );

    cancelButton.signal_clicked().connect([&]() {
            onResponse.emit(Gtk::ResponseType::CANCEL);
            hide();
        }
    );

    button.set_expand();
    grid.attach(button, 0, 0, 2);
    grid.attach(cancelButton, 0, 1);
    grid.attach(okButton, 1, 1);
    grid.set_expand();
    set_child(grid);
}

unsigned int PortSelector::getValue() const {
    return button.get_value();
}

sigc::signal<void(Gtk::ResponseType)> PortSelector::signalOnResponse() const {
    return onResponse;
}
