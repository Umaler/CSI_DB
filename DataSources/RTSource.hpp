#pragma once

#include "DataSource.hpp"
#include <atomic>
#include <array>
#include <mutex>
#include <thread>
#include <glibmm/dispatcher.h>
#include <gtkmm/box.h>
#include <gtkmm/frame.h>
#include <gtkmm/entry.h>
#include <gtkmm/button.h>

class RTSource : public DataSource {
public:
    RTSource(std::string dbFilename = "", unsigned int newPort = 40055);

    virtual const DBDescriptor& getDescriptor() const;

    virtual void setBoundaries(Boundaries bounds);
    virtual void addCollectionType(std::string table, std::string field);
    virtual void removeCollectionType(std::string table, std::string field);
    virtual void removeAllCollectionTypes();
    virtual void stopCollection();

    virtual DataSource::SignalType signalOnNewDataArrived() const;

    virtual ~RTSource() = default;

private:
    Gtk::Frame entryFrame;
    Gtk::Box markBox;
    Gtk::Entry markEntry;
    Gtk::Frame lastMarkFrame;
    Gtk::Entry lastMarkEntry;
    Gtk::Button applyButton{"Применить маркировку"};

    std::mutex markM;
    std::atomic<bool> markUpdated;
    Glib::ustring mark;

    void newDataArrived();

    std::mutex filtersM;
    std::atomic<bool> filtersUpdated;
    Boundaries bounds;
    std::string table, field;

    std::mutex commonBufM;
    std::vector<std::vector<std::pair<double, double>>> commonBuf;

    DataSource::SignalType newDataArrivedSignal;
    Glib::Dispatcher newDataCollected;

    const unsigned int port;

    class Worker {
    public:
        Worker(RTSource& par, unsigned int p, SQLite::Database&& ndb);

        void start();
        void stop();

    private:
        struct DataSlice {
            double amps[3][3];
            double phas[3][3];
        };

        struct FullDataSlice : DataSlice {
            double imag[3][3];
            double real[3][3];
        };

        SQLite::Database db;

        RTSource& parent;
        const unsigned int port;
        std::unique_ptr<std::thread, std::function<void(std::thread*)>> workerThread;
        std::atomic<bool> pauseTransfer;
        std::atomic<bool> stopWorker;
        void writeToDB(const FullDataSlice& slice, size_t subcarr);
        void work();
    } worker;

    inline static const DBDescriptor desc
    {
        {
            "amplitude",
            {
                "11",
                "12",
                "13",
                "21",
                "22",
                "23",
                "31",
                "32",
                "33"
            }
        },
        {
            "phase",
            {
                "11",
                "12",
                "13",
                "21",
                "22",
                "23",
                "31",
                "32",
                "33"
            }
        }
    };

};
