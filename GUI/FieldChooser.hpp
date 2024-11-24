#pragma once

#include <gtkmm.h>
#include "../utils/DBDescriptor.hpp"

class FieldChooser : public Gtk::ScrolledWindow {
public:
    FieldChooser(const DBDescriptor& dbDesc);

    using SignalOnChoosed = sigc::signal<void(Glib::ustring, Glib::ustring)>; //table name, field name
    SignalOnChoosed& signalOnChoosed();

private:
    void onSelected(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn*);

    SignalOnChoosed signalChoosed;

    class ModelColumns : public Gtk::TreeModel::ColumnRecord
    {
    public:
        ModelColumns()
        { add(sourceName); }

        Gtk::TreeModelColumn<Glib::ustring> sourceName;
        inline static const Glib::ustring sourceColumnName = "Источники данных";

    } columnName;

    Glib::RefPtr<Gtk::TreeStore> sourcesTreeContent;
    Gtk::TreeView sourcesTree;

};
