#include "DBDescriptor.hpp"

#include <SQLiteCpp/Statement.h>

DBDescriptor::Table::Fields::iterator DBDescriptor::Table::begin() {
    return fields.begin();
}

DBDescriptor::Table::Fields::const_iterator DBDescriptor::Table::begin() const {
    return fields.begin();
}

DBDescriptor::Table::Fields::iterator DBDescriptor::Table::end() {
    return fields.end();
}

DBDescriptor::Table::Fields::const_iterator DBDescriptor::Table::end() const {
    return fields.end();
}

DBDescriptor::DBDescriptor(std::initializer_list<Table> il) {
    for(const auto& i : il) {
        tables.push_back(i);
    }
}

DBDescriptor::DBDescriptor(const DBDescriptor&) = default;

DBDescriptor DBDescriptor::add(std::initializer_list<Table> il) const {
    DBDescriptor newdesc(*this);

    for(const auto& i : il) {
        newdesc.tables.push_back(i);
    }
    return newdesc;
}

DBDescriptor::Tables::iterator DBDescriptor::begin() {
    return tables.begin();
}

DBDescriptor::Tables::const_iterator DBDescriptor::begin() const {
    return tables.begin();
}

DBDescriptor::Tables::iterator DBDescriptor::end() {
    return tables.end();
}

DBDescriptor::Tables::const_iterator DBDescriptor::end() const {
    return tables.end();
}

bool DBDescriptor::checkCompliance(SQLite::Database& db) const {
    for(const auto& i : tables) {
        if(!db.tableExists(i.name))  //no such table
            return false;

        for(const auto& j : i.fields) {
            SQLite::Statement querryCheckFields(db, "SELECT EXISTS(SELECT * FROM pragma_table_info(?) WHERE name=?)");
            querryCheckFields.bind(1, i.name);
            querryCheckFields.bind(2, j);
            if(!querryCheckFields.executeStep() || querryCheckFields.getColumn(0).getInt() == 0) //no such field
                return false;
        }
    }

    return true;
}
