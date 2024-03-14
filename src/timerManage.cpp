#include "timerManage.h"

TimerManage::TimerManage() {
    heap_.resize(64);
}

TimerManage::~TimerManage() {
    clear();
}

// 交换两个节点，并且要把ref_索引交换
void TimerManage::swapNode_(size_t i,size_t j) {
    assert(i >= 0 && i < heap_.size());
    assert(j >= 0 && j < heap_.size());
    swap(heap_[i],heap_[j]);
    ref_[heap_[i].id] = j;
    ref_[heap_[j].id] = i;
}

void TimerManage::siftUp_(size_t child) {
    assert(child >= 0 && child < heap_.size());
    size_t parent = (child - 1) >> 1;
    while (child > 0) {
        if (heap_[parent] > heap_[child]) {
            swapNode_(child,parent);
            child = parent;
            parent = (child - 1) >> 1;
        } else {
            break;
        }
    }
}

bool TimerManage::siftDown_(size_t parent,size_t n) {
    int child = (parent << 1) | 1,idx = parent;
    while (child < n) {
        if (child + 1 < n && heap_[child] > heap_[child + 1]) {
            child ++;
        }
        if (heap_[child] < heap_[parent]) {
            swapNode_(idx,child);
            idx = child;
            child = (child << 1) | 1;
        }
        break;
    }
    return idx > parent;
}

// 删除指定位置得结点
void TimerManage::del_(size_t index) {
    assert(index >= 0 && index < heap_.size());
    size_t n = heap_.size() - 1;
    if (index < n) {
        swapNode_(index,n);
        if (!siftDown_(index,n)) {
            siftUp_(index);
        }
    }
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}

// 调整指定id得结点
void TimerManage::adjust(int id,int newExpires) {
    assert(!heap_.empty() && ref_.count(id));
    heap_[ref_[id]].expires = Clock::now() + ms(newExpires);
    siftDown_(ref_[id],heap_.size());
}

void TimerManage::add(int id,int timeOut,const timeoutCallBack& cb) {
    assert(id >= 0);
    if (ref_.count(id)) {
        int tmp = ref_[id];
        heap_[tmp].expires = Clock::now() + ms(timeOut);
        heap_[tmp].cb = cb;
        if (!siftDown_(tmp,heap_.size())) {
            siftUp_(tmp);
        }
    } else {
        size_t n = heap_.size();
        ref_[id] = n;
        heap_.push_back({id,Clock::now() + ms(timeOut),cb});
        siftUp_(n);
    }
}

// 删除指定id，并触发回调函数
void TimerManage::doWork(int id) {
    if (heap_.empty() || !ref_.count(id)) {
        return;
    }
    size_t i = ref_[id];
    auto node = heap_[i];
    node.cb();
    del_(i);
}

// 起搏函数，清除超时结点
void TimerManage::tick() {
    if (heap_.empty()) {
        return;
    }
    while (!heap_.empty()) {
        TimerNode node = heap_.front();
        if (std::chrono::duration_cast<ms>(node.expires - Clock::now()).count() > 0) {
            break;
        }
        node.cb();
        pop();
    }
}

void TimerManage::pop() {
    assert(!heap_.empty());
    del_(0);
}

void TimerManage::clear() {
    ref_.clear();
    heap_.clear();
}

int TimerManage::getNextTick() {
    tick();
    size_t res = -1;
    if (!heap_.empty()) {
        res = std::chrono::duration_cast<ms>(heap_.front().expires - Clock::now()).count();
        if (res < 0) {
            res = 0;
        }
    }
    return res;
}