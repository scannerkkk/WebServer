#ifndef TIMER_MANAGE_H
#define TIMER_MANAGE_H

#include <queue>
#include <unordered_map>
#include <ctime>
#include <algorithm>
#include <arpa/inet.h>
#include <functional>
#include <cassert>
#include <chrono>
#include "log.h"

typedef std::function<void()> timeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds ms;
typedef Clock::time_point timeStamp;

struct TimerNode {
    int id;
    timeStamp expires;
    timeoutCallBack cb;
    bool operator< (const TimerNode& other) const {
        return expires < other.expires;
    }
    bool operator> (const TimerNode& other) const {
        return expires > other.expires;
    }
};

class TimerManage {
public:
    TimerManage();
    ~TimerManage();

    void adjust(int,int);
    void add(int,int,const timeoutCallBack&);
    void doWork(int);
    void clear();
    void tick();
    void pop();
    int getNextTick();

private:
    void del_(size_t);
    void siftUp_(size_t);
    bool siftDown_(size_t,size_t);
    void swapNode_(size_t,size_t);

    std::vector<TimerNode> heap_;
    std::unordered_map<int,size_t> ref_;
};
#endif