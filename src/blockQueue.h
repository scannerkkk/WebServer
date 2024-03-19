#ifndef BOLCK_QUEUE_H
#define BOLCK_QUEUE_H

#include <deque>
#include <condition_variable>
#include <mutex>
#include <sys/time.h>
#include <cassert>

template <class T>
class BlockQueue {
public:
    explicit BlockQueue(size_t maxsize = 1000);
    ~BlockQueue();

    bool empty();
    bool full();
    void push_back(const T& item);
    void push_front(const T& item);
    bool pop(T& item);
    bool pop(T& item,int timeout);
    void clear();

    T front();
    T back();
    size_t capacity();
    size_t size();

    void flush();
    void close();

private:
    std::deque<T> deq_;          
    std::mutex mut_;         
    bool isClose_;
    size_t capacity_;
    std::condition_variable consumer_;
    std::condition_variable producer_;
};

#endif