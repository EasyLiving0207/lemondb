//
// Created by liu on 18-10-23.
//

#ifndef PROJECT_DB_H
#define PROJECT_DB_H

#include <memory>
#include <memory>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <queue>
#include <map>
#include <atomic>
#include <condition_variable>

#include "Table.h"
#include "../query/Query.h"
#include "../query/QueryResult.h"
#include <semaphore.h>

//static unsigned int threadNum;

class Database {
private:
    /**
     * A unique pointer to the global database object
     */
    static std::unique_ptr<Database> instance;

    /**
     * The map of tableName -> table unique ptr
     */
    std::unordered_map<std::string, Table::Ptr> tables;
    std::unordered_map<std::string, std::unique_ptr<std::mutex>> tableLocks;
    std::unordered_map<std::string, std::unique_ptr<std::mutex>> writerLocks;
    std::unordered_map<std::string, std::unique_ptr<std::mutex>> readerLocks;
    std::unordered_map<std::string, std::unique_ptr<std::mutex>> counterLocks;
//    std::unordered_map<std::string, std::queue<size_t>> tableLocksId;
    std::unordered_map<std::string, size_t> counters;
    std::queue<size_t> queries;

    /**
     * The map of fileName -> tableName
     */
    std::unordered_map<std::string, std::string> fileTableNameMap;

    /**
     * The default constructor is made private for singleton instance
     */
    Database() = default;

    /**
     * Number of threads
     */
    unsigned int threadNum;

public:

    std::mutex mtx;
    std::condition_variable cv;

    void testDuplicate(const std::string &tableName);

    Table &registerTable(Table::Ptr &&table);

    void dropTable(const std::string &tableName);

    void truncateTable(const std::string &tableName);

    void printAllTable();

    Table &operator[](const std::string &tableName);

    const Table &operator[](const std::string &tableName) const;

    Database &operator=(const Database &) = delete;

    Database &operator=(Database &&) = delete;

    Database(const Database &) = delete;

    Database(Database &&) = delete;

    ~Database() = default;

    static Database &getInstance();

    void updateFileTableName(const std::string &fileName, const std::string &tableName);

    std::string getFileTableName(const std::string &fileName);

    void setThreadNum(unsigned int setThreadNum) {
        this->threadNum = setThreadNum;
    }

    unsigned int getThreadNum() {
        return this->threadNum;
    }

    /**
     * Load a table from an input stream (i.e., a file)
     * @param is
     * @param source
     * @return reference of loaded table
     */
    Table &loadTableFromStream(std::istream &is, std::string source = "");

    inline void addLock(const std::string &targetTable) {
        if (tableLocks.count(targetTable) == 0) {
            tableLocks.emplace(targetTable, std::make_unique<std::mutex>());
            writerLocks.emplace(targetTable, std::make_unique<std::mutex>());
            readerLocks.emplace(targetTable, std::make_unique<std::mutex>());
            counterLocks.emplace(targetTable, std::make_unique<std::mutex>());
            counters.emplace(targetTable, 0);
        }
    }

//    inline void lockTable(const std::string &targetTable, size_t id) {
//        if (tableLocks.count(targetTable) == 0) {
//            tableLocks.emplace(targetTable, std::make_unique<std::mutex>());
//            std::queue<size_t> newTable;
//            tableLocksId.emplace(targetTable, std::move(newTable));
//        }
//        tableLocksId[targetTable].push(id);
//    }

//    inline void unlockTable(const std::string &targetTable) {
//        tableLocksId[targetTable].pop();
//    }

    inline bool isReady(const size_t id) {
        return queries.front() == id;
    }

//    inline void waitLock(const std::string &targetTable, size_t id) {
//        std::unique_lock<std::mutex> lck(*tableLocks[targetTable]);
//        cv.wait(lck, [&] { return tableLocksId[targetTable].front() == id; });
//    }

    inline void readLock(const std::string &targetTable) {
        addLock(targetTable);
        readerLocks[targetTable]->lock();
        counterLocks[targetTable]->lock();
        if (counters[targetTable]++ == 0)
            tableLocks[targetTable]->lock();
        counterLocks[targetTable]->unlock();
        readerLocks[targetTable]->unlock();
    }

    inline void readUnlock(const std::string &targetTable) {
        counterLocks[targetTable]->lock();
        if (--counters[targetTable] == 0)
            tableLocks[targetTable]->unlock();
        counterLocks[targetTable]->unlock();
    }

    inline void writeLock(const std::string &targetTable) {
        addLock(targetTable);
        readerLocks[targetTable]->lock();
        tableLocks[targetTable]->lock();
        readerLocks[targetTable]->unlock();
    }

    inline void writeUnlock(const std::string &targetTable) {
        tableLocks[targetTable]->unlock();
    }

    inline void addQuery(const size_t id) {
        queries.push(id);
    }

    inline void moveQuery() {
        queries.pop();
    }

    inline void exit() {
        std::exit(0);
    }
};


#endif //PROJECT_DB_H