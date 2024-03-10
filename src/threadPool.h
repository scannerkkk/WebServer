#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <cassert>
// #include <iostream>

class ThreadPool {
public:
    ThreadPool() = default;
    ThreadPool(ThreadPool&&) = default;

    explicit ThreadPool(int threadCount = 8) : pool_(std::make_shared<Pool>()) {
        assert(threadCount > 0);
        for (int i = 0;i < threadCount;i ++) {
            std::thread([this]() { // 捕获this指针，在当前lambda中可以调用该类得成员函数和成员变量
                std::unique_lock<std::mutex> locker(pool_->mut_);
                while (true) {
                    if (!pool_->tasks.empty()) {
                        auto task = std::move(pool_->tasks.front());
                        pool_->tasks.pop();
                        locker.unlock(); // 取出来之后就可以解锁了
                        task();
                        locker.lock(); // 执行完后重新上锁
                    } else if (pool_->isClose) {
                        break;
                    } else {
                        pool_->cond_.wait(locker);
                    }
                }
            }).detach();
        }
    }

    ~ThreadPool();

    template <class T>
    void AddTask(T&& task);
private:
    struct Pool {
        std::mutex mut_;
        std::condition_variable cond_;
        bool isClose;
        std::queue<std::function<void()>> tasks;
    };
    std::shared_ptr<Pool> pool_;
};

ThreadPool::~ThreadPool() {
    if (pool_) {
        std::unique_lock<std::mutex> locker(pool_->mut_);
        pool_->isClose = true;
    }
    pool_->cond_.notify_all();
}

template <class T>
void ThreadPool::AddTask(T&& task) {
    // std::cout << "Add" << std::endl;
    std::unique_lock<std::mutex> locker(pool_->mut_);
    pool_->tasks.emplace(std::forward<T&&>(task));
    pool_->cond_.notify_one();
}
#endif