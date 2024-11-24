#pragma once

#include <gtkmm.h>

#include "../utils/Boundaries.hpp"

class ChooserLimiter : public Gtk::Grid {
    public:
        ChooserLimiter(Glib::ustring fieldName);

        std::pair<int64_t, int64_t> getLimits() const;

        sigc::signal<void()> signalOnUpdate() const;

    private:
        sigc::signal<void()> updateSignal;

        void onUpdate();

        Glib::RefPtr<Gtk::Adjustment> bottomBoundAdj;
        Gtk::SpinButton bottomBoundButton;
        Gtk::Frame bottomBoundFrame;

        Glib::RefPtr<Gtk::Adjustment> topBoundAdj;
        Gtk::SpinButton topBoundButton;
        Gtk::Frame topBoundFrame;

        Gtk::CheckButton shouldChooseButton;

    };

class ChoosersPanel : public Gtk::Grid {
public:

    ChoosersPanel();

    Boundaries getBounds() const;

    std::pair<int64_t, int64_t> getIdLimits() const;
    std::pair<int64_t, int64_t> getidPacketLimits() const;
    std::pair<int64_t, int64_t> getIdMeasLimits() const;
    std::pair<int64_t, int64_t> getNumSubLimits() const;

    sigc::signal<void(Boundaries)> signalNewBounds() const;

private:
    sigc::signal<void(Boundaries)> updateSignal;

    ChooserLimiter idChooser, idPacketChooser, idMeasChooser;
    Glib::RefPtr<Gtk::Adjustment> numsubAdj;
    Gtk::SpinButton numsubButton;
    Gtk::Frame numsubFrame;

};
