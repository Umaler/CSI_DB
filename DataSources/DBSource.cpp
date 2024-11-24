#include "DBSource.hpp"

#include <iostream>

DBSource::DBSource(SQLite::Database&& database) :
    db(std::move(database)),

    collectionTypesUpdated(false),

    boundsUpdated(false),

    worker(*this)
{
    if(!dbDescriptor.checkCompliance(db)) {
        throw std::runtime_error("Non-compliant database");
    }

    workerGotData.connect([&](){
        std::lock_guard lg(commonBufferM);
        signalDataArrived.emit(std::move(commonBuffer));
        commonBuffer.clear();
    });
}

const DBDescriptor& DBSource::getDescriptor() const {
    return dbDescriptor;
}

void DBSource::setBoundaries(Boundaries bounds) {
    std::lock_guard lg(boundsM);
    this->bounds = bounds;
    boundsUpdated = true;

    worker.start();
}

void DBSource::addCollectionType(std::string table, std::string field) {
    std::lock_guard lg(collectionTypesM);
    collectionTypes.clear();
    collectionTypes.push_back({table, field});
    collectionTypesUpdated = true;

    worker.start();
}

void DBSource::removeCollectionType(std::string table, std::string field) {
    std::lock_guard lg(collectionTypesM);
    std::pair<std::string, std::string> toDelete{table, field};
    std::remove(collectionTypes.begin(), collectionTypes.end(), toDelete);
    collectionTypesUpdated = true;
}

void DBSource::removeAllCollectionTypes() {
    worker.stop();
    collectionTypes.clear();
}

void DBSource::stopCollection() {
    worker.stop();
}

DataSource::SignalType DBSource::signalOnNewDataArrived() const {
    return signalDataArrived;
}

DBSource::Worker::Worker(DBSource& parent) :
    dbSource(parent),
    workerThread(nullptr,
                 [&](std::thread* thr) {
                     if(!thr)
                         return;
                     stopWorker = true;
                     thr->join();
                     stopWorker = false;
                 }
                 ),
    stopWorker(false)
{
}

void DBSource::Worker::start() {
    workerThread.reset(new std::thread(&DBSource::Worker::work, this));
}

void DBSource::Worker::stop() {
    workerThread.reset();
    dbSource.commonBuffer.clear();
}

void DBSource::Worker::work() {
    const size_t bufSize = 100;

    auto getQuery = [&](std::string table, std::string field) {
        try {
            std::string request;
            request += "SELECT id, " + field + " FROM " + table + " WHERE id >= ? AND id <= ? AND id_packet >= ? AND id_packet <= ? AND id_measurement >= ? AND id_measurement <= ? AND num_sub >= ? AND num_sub <= ?";

            SQLite::Statement query(dbSource.db, request);
            query.bind(1, dbSource.bounds.id.min);     query.bind(2, dbSource.bounds.id.max);
            query.bind(3, dbSource.bounds.packId.min); query.bind(4, dbSource.bounds.packId.max);
            query.bind(5, dbSource.bounds.measId.min); query.bind(6, dbSource.bounds.measId.max);
            query.bind(7, dbSource.bounds.numSub.min); query.bind(8, dbSource.bounds.numSub.max);

            return query;
        }
        catch(std::exception& ex) {
            std::cerr << ex.what() << std::endl;
        }
        return SQLite::Statement(dbSource.db, "");
    };

    dbSource.collectionTypesM.lock();
    std::vector<SQLite::Statement> queries;
    queries.reserve(dbSource.collectionTypes.size());
    for(const auto& i : dbSource.collectionTypes) {
        queries.push_back(getQuery(i.first, i.second));
    }
    dbSource.collectionTypesM.unlock();
    std::vector<bool> correctQueries(queries.size(), true);
    size_t validQueries = queries.size();

    std::vector<std::vector<std::pair<double, double>>> localBuffer;

    while (!stopWorker) {
        localBuffer.resize(queries.size());
        for(size_t i = 0; i < queries.size(); i++) {
            if(!correctQueries[i])
                continue;

            auto& query = queries[i];
            for(size_t j = 0; j < bufSize; j++) {
                try {
                    if(!query.executeStep()) {
                        correctQueries[i] = false;
                        validQueries--;
                        break;
                    }
                    unsigned long long id = query.getColumn(0).getInt64();
                    double value = query.getColumn(1);
                    localBuffer[i].push_back({id, value});
                }
                catch(std::exception& ex) {
                    std::cerr << ex.what() << std::endl;
                    correctQueries[i] = false;
                    validQueries--;
                    break;
                }
            }
        }

        bool commonBufferFree = false;
        while(!commonBufferFree && !stopWorker) {
            dbSource.commonBufferM.lock();
            commonBufferFree = dbSource.commonBuffer.empty();
            if(commonBufferFree) {
                dbSource.commonBuffer = localBuffer;
                localBuffer.clear();
                dbSource.workerGotData.emit();
            }
            dbSource.commonBufferM.unlock();
        }

        if(validQueries == 0)
            break;
    }
}
