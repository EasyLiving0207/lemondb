//
// Created by liu on 18-10-21.
//

#include "query/QueryParser.h"
#include "query/QueryBuilders.h"
#include "db/Database.h"
#include "thread/ThreadPool.h"

#include <stack>
#include <getopt.h>
#include <fstream>
#include <iostream>
#include <string>

//const char *qnameString[] = {
//        "LOAD",
//        "DROP",
//        "TRUNCATE",
//        "DUMP",
//        "COPYTABLE",
//        "LIST",
//        "QUIT",
//        "SHOWTABLE",
//        "INSERT",
//        "UPDATE",
//        "SELECT",
//        "DELETE",
//        "DUPLICATE",
//        "COUNT",
//        "SUM",
//        "MIN",
//        "MAX",
//        "ADD",
//        "SUB",
//        "SWAP",
//        "LISTEN"
//};

enum op_type {
    READ, WRITE, NONE
};

struct {
    std::string listen;
    long threads = 0;
} parsedArgs;

void parseArgs(int argc, char *argv[]) {
    const option longOpts[] = {
            {"listen",  required_argument, nullptr, 'l'},
            {"threads", required_argument, nullptr, 't'},
            {nullptr,   no_argument,       nullptr, 0}
    };
    const char *shortOpts = "l:t:";
    int opt;
    int longIndex;
    while ((opt = getopt_long(argc, argv, shortOpts, longOpts, &longIndex)) != -1) {
        if (opt == 'l') {
            parsedArgs.listen = optarg;
        } else if (opt == 't') {
            parsedArgs.threads = std::strtol(optarg, nullptr, 10);
        } else {
            std::cerr << "lemondb: warning: unknown argument " << longOpts[longIndex].name << std::endl;
        }
    }

}

std::string extractQueryString(std::istream &is) {
    std::string buf;
    do {
        int ch = is.get();
        if (ch == ';') {
            return buf;
        }
        if (ch == EOF) {
            throw std::ios_base::failure("End of input");
        }
        buf.push_back(static_cast<char>(ch));
    } while (true);
}

int main(int argc, char *argv[]) {
    // Assume only C++ style I/O is used in lemondb
    // Do not use printf/fprintf in <cstdio> with this line
    std::ios_base::sync_with_stdio(false);

    parseArgs(argc, argv);
    std::stack<std::unique_ptr<std::fstream>> filestack;
    filestack.push(std::make_unique<std::fstream>());
    if (!parsedArgs.listen.empty()) {
        filestack.top()->open(parsedArgs.listen);
        if (!filestack.top()->is_open()) {
            std::cerr << "lemondb: error: " << parsedArgs.listen << ": no such file or directory" << std::endl;
            exit(-1);
        }
    }
    std::istream is(filestack.top()->rdbuf());

    Database &lemonDb = Database::getInstance();

#ifdef NDEBUG
    // In production mode, listen argument must be defined
    if (parsedArgs.listen.empty()) {
        std::cerr << "lemondb: error: --listen argument not found, not allowed in production mode" << std::endl;
        exit(-1);
    }
#else
    // In debug mode, use stdin as input if no listen file is found
    if (parsedArgs.listen.empty()) {
        std::cerr << "lemondb: warning: --listen argument not found, use stdin instead in debug mode" << std::endl;
        is.rdbuf(std::cin.rdbuf());
    }
#endif

    if (parsedArgs.threads < 0) {
        std::cerr << "lemondb: error: threads num can not be negative value " << parsedArgs.threads << std::endl;
        exit(-1);
    } else if (parsedArgs.threads == 0) {
        // @TODO Auto detect the thread num
        unsigned int threadNum = std::thread::hardware_concurrency();
        lemonDb.setThreadNum(threadNum);
        std::cerr << "lemondb: info: auto detect thread num " << threadNum << std::endl;
    } else {
        unsigned int threadNum = parsedArgs.threads;
        lemonDb.setThreadNum(threadNum);
        std::cerr << "lemondb: info: running in " << parsedArgs.threads << " threads" << std::endl;
    }

    QueryParser p;

    p.registerQueryBuilder(std::make_unique<QueryBuilder(Debug)>());
    p.registerQueryBuilder(std::make_unique<QueryBuilder(ManageTable)>());
    p.registerQueryBuilder(std::make_unique<QueryBuilder(Complex)>());

    ThreadPool pool(lemonDb.getThreadNum());
    std::vector<std::future<QueryResult::Ptr> > results;

    size_t counter = 0;
    while (is) {
        try {
            // A very standard REPL
            // REPL: Read-Evaluate-Print-Loop
            std::string queryStr = extractQueryString(is);
            Query::Ptr query = p.parseQuery(queryStr);
            std::string qname = query->getQname();
//            std::cout << qname << std::endl;
            if (qname == "QUIT") {
                break;
            }
            if (qname == "LISTEN") {
                std::string filename = query->getTableName();
                filestack.push(std::make_unique<std::fstream>());
                filestack.top()->open(filename);
                if (filestack.top()->is_open()) {
                    is.rdbuf(filestack.top()->rdbuf());
                } else
                    std::cerr << "lemondb: error: " << filename << ": no such file or directory" << std::endl;
            }

            op_type type;
            if (qname == "COPYTABLE" || qname == "DUMP" || qname == "SELECT" || qname == "SUM" || qname == "COUNT" ||
                qname == "MIN" || qname == "MAX")
                type = READ;
            else if (qname == "DROP" || qname == "TRUNCATE" || qname == "DELETE" || qname == "INSERT" ||
                     qname == "UPDATE" || qname == "SWAP" || qname == "DUPLICATE" || qname == "ADD" || qname == "SUB")
                type = WRITE;
            else
                type = NONE;

            //lemonDb.addQuery(counter);

            results.emplace_back(pool.enqueue([&lemonDb, query = std::move(query), type] {
                //temporary code, may be moved to query's function

//                std::unique_lock<std::mutex> lck(lemonDb.mtx);
//                lemonDb.cv.wait(lck, [&] { return lemonDb.isReady(counter); });
//                auto tableName = query->getTableName();
//                lemonDb.lockTable(query->getTableName(), counter);
                if (type == READ)
                    lemonDb.readLock(query->getTableName());
                else if (type == WRITE)
                    lemonDb.writeLock(query->getTableName());
//                std::cout << "executing" << std::endl;
//                lemonDb.moveQuery();
//                lemonDb.cv.notify_all();

//                lemonDb.waitLock(query->getTableName(), counter);
                auto res = query->execute();

                if (type == READ)
                    lemonDb.readUnlock(query->getTableName());
                else if (type == WRITE)
                    lemonDb.writeUnlock(query->getTableName());

                //lemonDb.unlockTable(query->getTableName());
                //lemonDb.cv.notify_all();

                return res;
            }));
            counter++;

        } catch (const std::ios_base::failure &e) {
            //end of input
            if (filestack.size() > 1) {
                filestack.pop();
                is.rdbuf(filestack.top()->rdbuf());
                continue;
            } else
                break;
        } catch (const std::exception &e) {
            std::cout.flush();
            std::cerr << e.what() << std::endl;
        }
    }

    counter = 0;
    for (auto &&result: results) {
        std::cout << ++counter << "\n";
        auto res = result.get();
        if (res->success()) {
            if (res->display()) {
                std::cout << *res;
            } else {
#ifndef NDEBUG
                std::cout.flush();
                std::cerr << *res;
#endif
            }
        } else {
            std::cout.flush();
            std::cerr << "QUERY FAILED:\n\t" << *res;
        }
    }

    return 0;
}