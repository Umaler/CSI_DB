#include "RTPhaseDiffSource.hpp"

RTPhaseDiffSource::RTPhaseDiffSource() : RTSource()
{
    RTSource::signalOnNewDataArrived().connect(sigc::mem_fun(*this, &RTPhaseDiffSource::onDataArrived));
}

void RTPhaseDiffSource::addCollectionType(std::string table, std::string field) {
    if(table == "phase")
        isPhaseMode = true;
    else
        isPhaseMode = false;
    RTSource::addCollectionType(table, field);
}

DataSource::SignalType RTPhaseDiffSource::signalOnNewDataArrived() const {
    return signalDataProcessed;
}

void RTPhaseDiffSource::onDataArrived(std::vector<std::vector<std::pair<double, double>>> data) {
    if(isPhaseMode) {
        for(auto i : data[0]) {
            double buf = i.second;
            i.second -= lastPhase;
            lastPhase = buf;
        }
    }
    signalDataProcessed.emit(std::move(data));
}
