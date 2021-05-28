//
// Created by  Easy Living on 11/10/20.
//

#ifndef P2_THREADPOOL_H
#define P2_THREADPOOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include "../db/Database.h"


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

void ThreadPool::getStarted() {
    std::unique_lock<std::mutex> lock(queueMutex);
    start = true;
    condition.notify_all();
}

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

#endif //P2_THREADPOOL_H