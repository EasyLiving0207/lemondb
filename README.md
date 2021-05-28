# LemonDB

## Version 2.0	Multithread

[TOC]

### Instructions

Currently we support 21 queries including:

```c++
"LOAD", "DROP", "TRUNCATE", "DUMP", "COPYTABLE", "LIST", "QUIT", "SHOWTABLE", "INSERT", "UPDATE", "SELECT", "DELETE", "DUPLICATE", "COUNT", "SUM", "MIN", "MAX", "ADD", "SUB", "SWAP", "LISTEN"
```

**Please refer to [p2.pdf](https://www.umjicanvas.com/courses/1826/files/409392/download?wrap=1) for detailed instructions!**

### Overall structure

```shell
~/Desktop/Files/FA2020/VE482/p2(wz*) » tree
.
├── CMakeLists.txt
├── README.md
├── db
│   ├── Database.cpp
│   ├── Database.h
│   ├── Table.cpp
│   └── Table.h
├── main.cpp
├── query
│   ├── Query.cpp
│   ├── Query.h
│   ├── QueryBuilders.cpp
│   ├── QueryBuilders.h
│   ├── QueryParser.cpp
│   ├── QueryParser.h
│   ├── QueryResult.cpp
│   ├── QueryResult.h
│   ├── data
│   │   ├── AddQuery.cpp
│   │   ├── AddQuery.h
│   │   ├── CountQuery.cpp
│   │   ├── CountQuery.h
│   │   ├── DeleteQuery.cpp
│   │   ├── DeleteQuery.h
│   │   ├── DuplicateQuery.cpp
│   │   ├── DuplicateQuery.h
│   │   ├── InsertQuery.cpp
│   │   ├── InsertQuery.h
│   │   ├── MaxQuery.cpp
│   │   ├── MaxQuery.h
│   │   ├── MinQuery.cpp
│   │   ├── MinQuery.h
│   │   ├── SelectQuery.cpp
│   │   ├── SelectQuery.h
│   │   ├── SubQuery.cpp
│   │   ├── SubQuery.h
│   │   ├── SumQuery.cpp
│   │   ├── SumQuery.h
│   │   ├── SwapQuery.cpp
│   │   ├── SwapQuery.h
│   │   ├── UpdateQuery.cpp
│   │   └── UpdateQuery.h
│   └── management
│       ├── CopyTableQuery.cpp
│       ├── CopyTableQuery.h
│       ├── DropTableQuery.cpp
│       ├── DropTableQuery.h
│       ├── DumpTableQuery.cpp
│       ├── DumpTableQuery.h
│       ├── ListTableQuery.cpp
│       ├── ListTableQuery.h
│       ├── ListenQuery.cpp
│       ├── ListenQuery.h
│       ├── LoadTableQuery.cpp
│       ├── LoadTableQuery.h
│       ├── PrintTableQuery.cpp
│       ├── PrintTableQuery.h
│       ├── QuitQuery.cpp
│       ├── QuitQuery.h
│       ├── TruncateTableQuery.cpp
│       └── TruncateTableQuery.h
├── thread
│   └── ThreadPool.h
└── utils
    ├── formatter.h
    └── uexception.h
```

Here is how the structure is organized:

+ `CMakeLists.txt`: stores the building environment configurations, thread libraries are dynamically linked, all `.h` and `.cpp` files are set as source. Default compiler is `clang`. Minimum version for `CMake` is 2.7.
+ `db/`: stores sources for database and table implementation.
+ `query/` all query executing classes, together with related assets including `QueryBuilder`, `QueryParser` and `QueryResult`. In detail, queries with data computations are put in `query/data/`, while queries with table management are put in `query/management`.
+ `thread/`, stores the thread pool class, which is used to implement multithreaded database.
+ `utils/`: stores the utilities ralated with string formatters and exception handlers.
+ `main.cpp`: main function for LemonDB.

### Thread pool

In version 2.0, we implemented the thread pool to realize multithreaded execution of queries.

#### Member functions

The source code of thread pool class is stored in `thread/ThreadPool.h`

```c++
class ThreadPool {
public:
    explicit ThreadPool(size_t);

    template<class F, class... Args>
    auto enqueue(F &&f, Args &&... args)
    -> std::future<typename std::result_of<F(Args...)>::type>;

    void getStarted();

    ~ThreadPool();

private:
    std::vector<std::thread> threads;
    std::queue<std::function<void()> > tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop;
    bool start;
};
```

#### Constructor and worker threads

On construction, `ThreadPool` class will allocate a vector of threads, depends on the number of thread detected by `std::thread::hardware_concurrency()`, or indicated by program arguments, together with a queue of tasks and a queue of results.

Each thread worker will follow a scheduler function and wait in a loop, which is

```c++
inline ThreadPool::ThreadPool(size_t threadNum)
        : stop(false), start(false) {
    for (size_t i = 0; i < threadNum; ++i) {
        threads.emplace_back(
                [this] {
                    //this->condition.wait(lock, [this] { return this->start; });
                    while (true) {
                        std::function<void()> task;
                        std::unique_lock<std::mutex> lock(this->queueMutex);
                        this->condition.wait(lock,
                                             [this] { return this->stop || !this->tasks.empty(); });
                        if (this->stop && this->tasks.empty()) { return; }
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                        task();
                    }
                }
        );
    }
}
```

A conditional variable is maintained to monitor the status of the queue. A thread worker hangs when the task queue is empty. When it is assigned to a task, it does its work and push the result the result queue, then loop again, until another task is pushed into the queue.

#### Enqueue function

Now we look at the `enqueue` function.

```c++
template<class F, class... Args>
auto ThreadPool::enqueue(F &&f, Args &&... args)
-> std::future<typename std::result_of<F(Args...)>::type> {

    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared<std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        if (stop) {
            throw std::runtime_error("enqueue on stopped thread pool");
        }
        tasks.emplace([task] { (*task)(); });
    }

    condition.notify_one();
    return res;
}
```

`enqueue` function takes a function to be executed(in the project it is the `Query::execute()` function), push it to the task queue, notify one thread worker to do the task, and return the result, since the task is not completed yet, the result is denoted by a `std::future` type.

#### Result queue

In `main.c`, the result of the execute fucntion is pushed to the `results` queue.

```c++
results.emplace_back(pool.enqueue([&lemonDb, query = std::move(query), type] {

    if (type == READ)
        lemonDb.readLock(query->getTableName());
    else if (type == WRITE)
        lemonDb.writeLock(query->getTableName());

    auto res = query->execute();

    if (type == READ)
        lemonDb.readUnlock(query->getTableName());
    else if (type == WRITE)
        lemonDb.writeUnlock(query->getTableName());

    return res;
}));
```

Later it is displayed by the order of the queue.

```c++
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
```

#### Destructor

The destructor simply sets `stop` to be true such that no more task can be pushed, and joins all the working threads.

```c++
inline ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        stop = true;
    }
    condition.notify_all();
    for (std::thread &worker: threads) {
        worker.join();
    }
}
```

### Reader-Writer problem

We refer to the reader-writer problem to implement mutex on the database.

#### Classification

Queries are splited into 3 classes, reader, writer and none, as follows. In `main.cpp`

```c++
enum op_type {
    READ, WRITE, NONE
};

op_type type;
if (qname == "COPYTABLE" || qname == "DUMP" || qname == "SELECT" || qname == "SUM" || qname == "COUNT" ||
    qname == "MIN" || qname == "MAX")
    type = READ;
else if (qname == "DROP" || qname == "TRUNCATE" || qname == "DELETE" || qname == "INSERT" ||
         qname == "UPDATE" || qname == "SWAP" || qname == "DUPLICATE" || qname == "ADD" || qname == "SUB")
    type = WRITE;
else
    type = NONE;
```

Those who modifies the table would be classified as writer, those who only reads from the table and compute results would be classified as reader, and those who does not have target table would be none.

The idea is for each table existing in the database, we create a mutex for it. While in the same moment a table can only be accessed by:

- One or more readers
- Only one writer
- None

While readers and writers from different tables are independent. To achieve this method, we use the reader-writer lock and unlock function.

#### Reader-Writer locks

In `Database.h`

```c++
std::unordered_map<std::string, std::unique_ptr<std::mutex>> tableLocks;
std::unordered_map<std::string, std::unique_ptr<std::mutex>> writerLocks;
std::unordered_map<std::string, std::unique_ptr<std::mutex>> readerLocks;
std::unordered_map<std::string, std::unique_ptr<std::mutex>> counterLocks;
std::unordered_map<std::string, size_t> counters;

inline void addLock(const std::string &targetTable) {
    if (tableLocks.count(targetTable) == 0) {
        tableLocks.emplace(targetTable, std::make_unique<std::mutex>());
        writerLocks.emplace(targetTable, std::make_unique<std::mutex>());
        readerLocks.emplace(targetTable, std::make_unique<std::mutex>());
        counterLocks.emplace(targetTable, std::make_unique<std::mutex>());
        counters.emplace(targetTable, 0);
    }
}

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
```

The reader and writer functions are shown previously in **Result Queue**.

### Performance

The test case is still small to show the improvements. Most time are consumed by `load` `copy` and `dump` functions, up tp about 80% of the whole running time. Since the database is an IO-bound process, our design is not best compatible with it. We may need __larger test case__s to maximize the performance. The following results are run on my 4 cores 8 threads MacBook Pro for `test.query`.

#### Multithread version

```shell
~/Desktop/Files/FA2020/VE482/p2/cmake-build-debug(wz*) » time ./lemondb --listen test.query
...
./lemondb --listen test.query  7.62s user 0.17s system 98% cpu 7.882 total
```

#### Singlethread version

```shell
~/Desktop/Files/FA2020/VE482/p2_single_thread/cmake-build-debug(master*) » time ./lemondb --listen test.query
...
./lemondb --listen test.query  8.62s user 0.30s system 98% cpu 9.029 total
```

### Problems remaining

The problem we still have will be the IO-bound of the process. For queries that need frequent loading and dumping on large tables, the advantage of the multithread pool is not revealed. So far we only let each thread deal with one query, improvements can be made by spliting single queries into multithreads, which requires further work and research.



