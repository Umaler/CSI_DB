#include "RTSource.hpp"

#include <iostream>
#include <chrono>
#include <sstream>
#include <cmath>

extern "C" {
    #include "../csi_fun.h"
}
#include <SFML/Network.hpp>
#include <SQLiteCpp/Transaction.h>

RTSource::RTSource(std::string dbFilename, unsigned int newPort) :
    markUpdated(false),
    entryFrame("Маркировка данных"),
    lastMarkFrame("Текущая маркировка"),

    port(newPort),
    filtersUpdated(false),
    worker(*this, port, SQLite::Database(dbFilename, SQLite::OPEN_READWRITE))
{
    newDataCollected.connect([&](){
        std::lock_guard lg(commonBufM);
        newDataArrivedSignal.emit(std::move(commonBuf));
        commonBuf.clear();
    });

    lastMarkEntry.set_editable(false);
    markBox.set_orientation(Gtk::Orientation::VERTICAL);
    markBox.append(markEntry);
    lastMarkFrame.set_child(lastMarkEntry);
    markBox.append(lastMarkFrame);
    markBox.append(applyButton);
    entryFrame.set_child(markBox);
    settingsBox.append(entryFrame);

    applyButton.signal_clicked().connect([&]() {
            Glib::ustring correctString;
            for(const char ch : markEntry.get_text()) {
                if(!std::isdigit(ch)) continue;
                correctString.push_back(ch);
            }

            lastMarkEntry.set_text(correctString);

            std::lock_guard lg(markM);
            markUpdated = true;
            mark = markEntry.get_text();
        }
    );
}

const DBDescriptor& RTSource::getDescriptor() const {
    return desc;
}

void RTSource::setBoundaries(Boundaries bounds) {
    std::lock_guard lg(filtersM);
    this->bounds = bounds;
    filtersUpdated = true;
    worker.start();
}

void RTSource::addCollectionType(std::string table, std::string field) {
    std::lock_guard lg(filtersM);
    std::lock_guard blg(commonBufM);
    this->table = table;
    this->field = field;
    filtersUpdated = true;

    commonBuf.clear();
    worker.start();
}

void RTSource::removeCollectionType(std::string table, std::string field) {
    removeAllCollectionTypes();
}

void RTSource::removeAllCollectionTypes() {
    std::lock_guard blg(commonBufM);
    commonBuf.clear();
    worker.stop();
}

void RTSource::stopCollection() {
    worker.stop();
}

DataSource::SignalType RTSource::signalOnNewDataArrived() const {
    return newDataArrivedSignal;
}

void RTSource::newDataArrived() {
    std::lock_guard lg(commonBufM);
    newDataArrivedSignal.emit(std::move(commonBuf));
    commonBuf.clear();
}

RTSource::Worker::Worker(RTSource& par, unsigned int p, SQLite::Database&& ndb) :
    db(std::move(ndb)),
    parent(par),
    port(p),
    pauseTransfer(true),
    stopWorker(false),

    workerThread(new std::thread(&RTSource::Worker::work, this),
                 [&](std::thread* thr) {
                    if(!thr)
                        return;
                    stopWorker = true;
                    thr->join();
                    stopWorker = false;
                 }
                 )
{
}

void RTSource::Worker::start() {
    pauseTransfer = false;
}

void RTSource::Worker::stop() {
    pauseTransfer = true;
}

void RTSource::Worker::writeToDB(const FullDataSlice& slice, size_t subcarr) {
    try {
        auto getMax = [&](std::string field, std::string table) -> int64_t {
            SQLite::Statement query(db, std::string("SELECT MAX(") + field + ") AS M FROM " + table);
            query.executeStep();
            return query.getColumn(0);
        };

        int64_t id_meas = getMax("id", "measurement") + 1;
        int64_t id_pack = getMax("id", "packet") + 1;

        auto nvars = [](size_t n) -> std::string {
            if(n == 0) return "";
            std::string result;
            for(size_t i = 0; i < n - 1; i++) {
                result += "?, ";
            }
            result += "?";
            return result;
        };

        auto constructAmpQ = [&](int64_t subc) {
            SQLite::Statement ampQ(db, "INSERT INTO amplitude VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
            ampQ.bind(1, id_meas);
            ampQ.bind(2, id_pack);
            ampQ.bind(3, id_meas);
            ampQ.bind(4, subc);
            return ampQ;
        };

        auto constructMeasQ = [&](int64_t subc) {
            SQLite::Statement measQ(db, std::string("INSERT INTO measurement VALUES (?, ?, ?, ") + nvars(18) + ")");
            measQ.bind(1, id_meas);
            measQ.bind(2, id_pack);
            measQ.bind(3, subc);
            return measQ;
        };

        auto constructPhaseQ = [&](int64_t subc) {
            SQLite::Statement phaseQ(db, "INSERT INTO phase VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
            phaseQ.bind(1, id_meas);
            phaseQ.bind(2, id_pack);
            phaseQ.bind(3, id_meas);
            phaseQ.bind(4, subc);
            return phaseQ;
        };

        auto timePointAsStr = []() {
            using namespace std::chrono;
            std::stringstream ss;
            ss << time_point_cast<seconds>(system_clock::now());
            return ss.str();
        };

        SQLite::Transaction transaction(db);
        // write packet
        SQLite::Statement packQ(db, "INSERT INTO packet VALUES(?, ?, ?)");
        packQ.bind(1, id_pack);
        parent.markM.lock();
            packQ.bind(2, parent.mark);
        parent.markM.unlock();
        packQ.bind(3, timePointAsStr());
        packQ.exec();

        auto amplQ = constructAmpQ(subcarr);
        auto phasQ = constructPhaseQ(subcarr);
        auto measQ = constructMeasQ(subcarr);
        for(int i = 0; i < 3; i++) {
            for(int j = 0; j < 3; j++) {
                amplQ.bind(5 + i * 3 + j, slice.amps[i][j]);
                phasQ.bind(5 + i * 3 + j, slice.phas[i][j]);
                measQ.bind(4 + (i * 3 + j) * 2, slice.real[i][j]);
                measQ.bind(5 + (i * 3 + j) * 2, slice.imag[i][j]);
            }
        }
        amplQ.exec();
        phasQ.exec();
        measQ.exec();
        transaction.commit();
    }
    catch(SQLite::Exception& ex) {
        std::cerr << "SQLite exception: " << ex.what() << std::endl;
    }
    catch(std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }
    catch(...) {
        std::cerr << "RTSource::Worker::writeToDB: unknown exception";
    }
}

void RTSource::Worker::work() {
    try {
        constexpr size_t maxSubcars = 114;
        std::list<std::array<DataSlice, maxSubcars>> allCollectedData;

        const size_t rawBufSize = sf::UdpSocket::MaxDatagramSize;    //maximal size of UDP datagram

        sf::UdpSocket socket;
        socket.setBlocking(false);

        // Listen to messages on the specified port
        if (socket.bind(port) != sf::Socket::Status::Done)
            return;

        unsigned char                in[rawBufSize];
        std::size_t                  received = 0;
        sf::IpAddress                sender;
        unsigned short               senderPort;

        Boundaries boundsToTransfer;
        size_t tx = 0, rx = 0;
        enum class DataType {
            amps,
            phas
        } dataToTransfer;

        COMPLEX csi_matrix[3][3][maxSubcars];
        csi_struct csi_status;
        while (!stopWorker) {
            std::vector<std::vector<std::pair<double, double>>> bufferToTransfer;
            bufferToTransfer.resize(1);
            if(parent.filtersUpdated) {
                { // update local copy of filters
                    std::lock_guard lg(parent.filtersM);
                    boundsToTransfer = parent.bounds;
                    tx = parent.field[0] - '1';
                    rx = parent.field[1] - '1';
                    if(parent.table == "amplitude") {
                        dataToTransfer = DataType::amps;
                    }
                    else if (parent.table == "phase") {
                        dataToTransfer = DataType::phas;
                    }
                }

                int64_t packid = boundsToTransfer.packId.min >= 0 ? boundsToTransfer.packId.min : 0;
                for(auto it = allCollectedData.begin();
                    packid <= boundsToTransfer.packId.max && it != allCollectedData.end();
                    packid++, ++it)
                {
                    for(int64_t subcarr = (boundsToTransfer.numSub.min >= 0 ? boundsToTransfer.numSub.min : 0);
                        subcarr < maxSubcars && subcarr <= boundsToTransfer.numSub.max;
                        subcarr++)
                    {
                        int64_t id = packid * maxSubcars + subcarr;
                        if(id < boundsToTransfer.id.min || id > boundsToTransfer.id.max)
                            continue;
                        if(dataToTransfer == DataType::amps) {
                            bufferToTransfer[0].push_back({id, (*it)[subcarr].amps[tx][rx]});
                        }
                        else {
                            bufferToTransfer[0].push_back({id, (*it)[subcarr].phas[tx][rx]});
                        }
                    }
                }

                parent.filtersUpdated = false;
            }

            sf::Socket::Status status = socket.receive(in, sizeof(in), received, sender, senderPort);
            if(status != sf::Socket::Status::Done)
                continue;

            record_status(in, received, &csi_status);

            if(csi_status.payload_len < 1056) {
                continue;
            }

            std::vector<unsigned char> data_buf(csi_status.payload_len);
            record_csi_payload(in, &csi_status, &data_buf[0], csi_matrix);

            allCollectedData.push_back({});
            DataSlice slice;
            for(unsigned i = 0; i < csi_status.num_tones; i++) {
                FullDataSlice fslice;
                for(unsigned j = 0; j < 3; j++) {
                    for(unsigned k = 0; k < 3; k++) {
                        int imag = csi_matrix[j][k][i].imag;
                        int real = csi_matrix[j][k][i].real;
                        fslice.imag[j][k] = imag;
                        fslice.real[j][k] = real;

                        // see CSI definition
                        slice.amps[j][k] = std::sqrt(imag * imag + real * real);
                        fslice.amps[j][k] = slice.amps[j][k];
                        if(real != 0) {
                            slice.phas[j][k] = std::atan(static_cast<double>(imag) / static_cast<double>(real));
                            if(real < 0 && imag >= 0)
                                slice.phas[j][k] += 3.14;
                            if(real < 0 && imag < 0)
                                slice.phas[j][k] -= 3.14;
                        }
                        else {
                            if(imag > 0) {
                                slice.phas[j][k] = 3.14 / 2.0;
                            }
                            else {
                                slice.phas[j][k] = -3.14 / 2.0;
                            }
                        }
                        fslice.phas[j][k] = slice.phas[j][k];
                    }
                }
                writeToDB(fslice, i);

                allCollectedData.back()[i] = slice;

                if(allCollectedData.size() < boundsToTransfer.packId.min || allCollectedData.size() > boundsToTransfer.packId.max)
                    continue;
                if(allCollectedData.size() * maxSubcars < boundsToTransfer.id.min || allCollectedData.size() * maxSubcars > boundsToTransfer.id.max)
                    continue;
                if(allCollectedData.size() * maxSubcars + i < boundsToTransfer.measId.min || allCollectedData.size() * maxSubcars + i > boundsToTransfer.measId.max)
                    continue;

                size_t x = allCollectedData.size() * maxSubcars + i;
                if(i >= boundsToTransfer.numSub.min && i <= boundsToTransfer.numSub.max) {
                    if(dataToTransfer == DataType::amps)
                        bufferToTransfer[0].push_back({x, slice.amps[tx][rx]});
                    else
                        bufferToTransfer[0].push_back({x, slice.phas[tx][rx]});
                }

            }

            if(pauseTransfer) {
                continue;
            }

            while(true) {
                std::lock_guard lg(parent.commonBufM);
                if(parent.commonBuf.empty()) {
                    std::swap(parent.commonBuf, bufferToTransfer);
                    parent.newDataCollected.emit();
                    break;
                }
            }
        }
    }
    catch(std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }
    catch(...) {
        std::cerr << "RTSource: unknown error" << std::endl;
    }
}
