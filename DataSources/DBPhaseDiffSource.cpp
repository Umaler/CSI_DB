#include "DBPhaseDiffSource.hpp"

DBPhaseDiffSource::DBPhaseDiffSource(SQLite::Database&& db) : DBSource(std::move(db))
{
    DBSource::signalOnNewDataArrived().connect(sigc::mem_fun(*this, &DBPhaseDiffSource::onDataArrived));
}

void DBPhaseDiffSource::addCollectionType(std::string table, std::string field) {
    if(table == "phase" || table == "clear_phase")
        isPhaseMode = true;
    else
        isPhaseMode = false;
    DBSource::addCollectionType(table, field);
}

DataSource::SignalType DBPhaseDiffSource::signalOnNewDataArrived() const {
    return signalDataProcessed;
}

void DBPhaseDiffSource::onDataArrived(std::vector<std::vector<std::pair<double, double>>> data) {
    if(isPhaseMode) {
        for(auto& i : data[0]) {
            double buf = i.second;
            i.second -= lastPhase;
            lastPhase = buf;
        }
    }
    signalDataProcessed.emit(std::move(data));
}

