#include "ChoosersPanel.hpp"

ChoosersPanel::ChoosersPanel() :
    idChooser("id"),
    idPacketChooser("id пакета"),
    idMeasChooser("id измерения"),

    numsubAdj(Gtk::Adjustment::create(0, 0, std::numeric_limits<int64_t>::max(), 1, 1)),
    numsubButton(numsubAdj),
    numsubFrame("Поднесущая")
{
    set_column_spacing(5);
    set_row_spacing(5);

    numsubFrame.set_child(numsubButton);

    attach(idChooser,       0, 0);
    attach(idPacketChooser, 1, 0);
    attach(idMeasChooser,   0, 1);
    attach(numsubFrame,     1, 1);

    auto onChooserUpdate = [&]() {
        updateSignal.emit(getBounds());
    };
    idChooser.signalOnUpdate().connect(onChooserUpdate);
    idPacketChooser.signalOnUpdate().connect(onChooserUpdate);
    idMeasChooser.signalOnUpdate().connect(onChooserUpdate);
    numsubButton.signal_value_changed().connect(onChooserUpdate);
}

Boundaries ChoosersPanel::getBounds() const {
    Boundaries bounds;

    auto getBound = [](const std::pair<int64_t, int64_t> pair) {
        Boundary bound;
        bound.min = pair.first;
        bound.max = pair.second;
        return bound;
    };

    bounds.id = getBound(getIdLimits());
    bounds.packId = getBound(getidPacketLimits());
    bounds.measId = getBound(getIdMeasLimits());
    bounds.numSub = getBound(getNumSubLimits());

    return bounds;
}

std::pair<int64_t, int64_t> ChoosersPanel::getIdLimits() const {
    return idChooser.getLimits();
}

std::pair<int64_t, int64_t> ChoosersPanel::getidPacketLimits() const {
    return idPacketChooser.getLimits();
}

std::pair<int64_t, int64_t> ChoosersPanel::getIdMeasLimits() const {
    return idMeasChooser.getLimits();
}

std::pair<int64_t, int64_t> ChoosersPanel::getNumSubLimits() const {
    size_t val = numsubButton.get_value_as_int();
    return {val, val};
}

sigc::signal<void(Boundaries)> ChoosersPanel::signalNewBounds() const {
    return updateSignal;
}

ChooserLimiter::ChooserLimiter(Glib::ustring fieldName) :
    bottomBoundAdj(Gtk::Adjustment::create(0, 0, std::numeric_limits<int64_t>::max(), 1, 1)),
    bottomBoundButton(bottomBoundAdj),
    bottomBoundFrame("Нижняя граница"),

    topBoundAdj(Gtk::Adjustment::create(0, 0, std::numeric_limits<int64_t>::max(), 1, 1)),
    topBoundButton(topBoundAdj),
    topBoundFrame("Верхняя граница"),

    shouldChooseButton(Glib::ustring("Ограничить ") + fieldName + "?")
{
    bottomBoundFrame.set_child(bottomBoundButton);
    bottomBoundButton.signal_value_changed().connect(sigc::mem_fun(*this, &ChooserLimiter::onUpdate));
    attach(bottomBoundFrame, 0, 0);
    bottomBoundFrame.set_hexpand();

    topBoundFrame.set_child(topBoundButton);
    topBoundButton.signal_value_changed().connect(sigc::mem_fun(*this, &ChooserLimiter::onUpdate));
    attach(topBoundFrame, 1, 0);
    topBoundFrame.set_hexpand();

    attach(shouldChooseButton, 0, 1, 2);
    shouldChooseButton.set_hexpand();
    shouldChooseButton.signal_toggled().connect([&](){updateSignal.emit();});

    set_hexpand();
}

std::pair<int64_t, int64_t> ChooserLimiter::getLimits() const {
    if(!shouldChooseButton.get_active())
        return {0, std::numeric_limits<int64_t>::max()};
    return {bottomBoundButton.get_value_as_int(), topBoundButton.get_value_as_int()};
}

sigc::signal<void()> ChooserLimiter::signalOnUpdate() const {
    return updateSignal;
}

void ChooserLimiter::onUpdate() {
    if(shouldChooseButton.get_active())
        updateSignal.emit();
}
