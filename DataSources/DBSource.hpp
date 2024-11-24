#pragma once

#include "DataSource.hpp"
#include <atomic>
#include <thread>
#include <memory>
#include <mutex>
#include <functional>
#include <glibmm/dispatcher.h>
#include <SQLiteCpp/Database.h>

class DBSource : public DataSource {
public:
    DBSource(SQLite::Database&& database);

    virtual const DBDescriptor& getDescriptor() const;

    virtual void setBoundaries(Boundaries bounds);
    virtual void addCollectionType(std::string table, std::string field);
    virtual void removeCollectionType(std::string table, std::string field);
    virtual void removeAllCollectionTypes();
    virtual void stopCollection();

    virtual SignalType signalOnNewDataArrived() const;

    virtual ~DBSource() = default;

private:
    SQLite::Database db;

    class Worker {
    public:
        Worker(DBSource& parent);

        void start();
        void stop();

    private:
        DBSource& dbSource;

        std::unique_ptr<std::thread, std::function<void(std::thread*)>> workerThread;
        std::atomic<bool> stopWorker;
        void work();

    } worker;

    Glib::Dispatcher workerGotData;
    std::mutex commonBufferM;
    std::vector<std::vector<std::pair<double, double>>> commonBuffer;

    std::atomic<bool> collectionTypesUpdated;
    std::mutex collectionTypesM;
    std::vector<std::pair<std::string, std::string>> collectionTypes;

    std::atomic<bool> boundsUpdated;
    std::mutex boundsM;
    Boundaries bounds;

    DataSource::SignalType signalDataArrived;

};
