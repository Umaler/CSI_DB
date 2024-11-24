#pragma once

#include <sigc++/sigc++.h>
#include <vector>
#include <string>
#include <utility>
#include <gtkmm/box.h>

#include "../utils/DBDescriptor.hpp"
#include "../utils/Boundaries.hpp"

class DataSource {
public:
    typedef sigc::signal<void(std::vector<std::vector<std::pair<double, double>>>)> SignalType;

    virtual const DBDescriptor& getDescriptor() const = 0;

    virtual void setBoundaries(Boundaries bounds) = 0;
    virtual void addCollectionType(std::string table, std::string field) = 0;
    virtual void removeCollectionType(std::string table, std::string field) = 0;
    virtual void removeAllCollectionTypes() = 0;
    virtual void stopCollection() = 0;

    virtual Gtk::Box& getSettingsBox() {
        return settingsBox;
    }

    virtual SignalType signalOnNewDataArrived() const = 0;

    virtual ~DataSource() = default;

protected:
    Gtk::Box settingsBox;

};
