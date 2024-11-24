#include "RTSource.hpp"

class RTPhaseDiffSource : public RTSource {
public:
    RTPhaseDiffSource();

    virtual void addCollectionType(std::string table, std::string field);

    virtual DataSource::SignalType signalOnNewDataArrived() const;

    ~RTPhaseDiffSource() = default;

private:
    DataSource::SignalType signalDataProcessed;

    bool isPhaseMode = false;
    double lastPhase = 0;
    void onDataArrived(std::vector<std::vector<std::pair<double, double>>> data);

};
