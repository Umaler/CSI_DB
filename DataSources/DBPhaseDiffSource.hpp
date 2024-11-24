#pragma once

#include "DBSource.hpp"

class DBPhaseDiffSource : public DBSource {
public:
    DBPhaseDiffSource(SQLite::Database&& db);

    virtual void addCollectionType(std::string table, std::string field);

    virtual DataSource::SignalType signalOnNewDataArrived() const;

    ~DBPhaseDiffSource() = default;

private:
    DataSource::SignalType signalDataProcessed;

    bool isPhaseMode = false;
    double lastPhase = 0;
    void onDataArrived(std::vector<std::vector<std::pair<double, double>>> data);

};

