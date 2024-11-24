#include "FieldChooser.hpp"

FieldChooser::FieldChooser(const DBDescriptor& dbDesc) {
    sourcesTreeContent = Gtk::TreeStore::create(columnName);
    sourcesTree.set_model(sourcesTreeContent);

    // construct selector of sources
    for(const auto& table : dbDesc) {
        auto row = *(sourcesTreeContent->append());
        row[columnName.sourceName] = table.name;
        for(const auto& field : table) {
            auto childrow = *(sourcesTreeContent->append(row.children()));
            childrow[columnName.sourceName] = field;
        }
    }

    sourcesTree.append_column(ModelColumns::sourceColumnName, columnName.sourceName);

    set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::ALWAYS);
    set_child(sourcesTree);

    sourcesTree.signal_row_activated().connect(sigc::mem_fun(*this, &FieldChooser::onSelected));
}

FieldChooser::SignalOnChoosed& FieldChooser::signalOnChoosed() {
    return signalChoosed;
}

void FieldChooser::onSelected(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn*) {
    auto iter = sourcesTreeContent->get_iter(path);
    if(!iter)
        return;
    Glib::ustring field = (*iter)[columnName.sourceName];
    auto upPath = path;
    if(upPath.up())
        iter = sourcesTreeContent->get_iter(upPath);
    if(!iter)
        return;
    Glib::ustring table = (*iter)[columnName.sourceName];

    signalChoosed.emit(table, field);
}
