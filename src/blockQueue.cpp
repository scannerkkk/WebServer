#include "blockQueue.h"
using namespace std;

template <class T>
BlockQueue<T>::BlockQueue(size_t maxsize) : capacity_(maxsize) {
    assert(maxsize > 0);
    isClose_ = false;
}

template <class T>
BlockQueue<T>::~BlockQueue() {
    close();
}

// lock before operation
template <class T>
void BlockQueue<T>::close() {
    clear();
    isClose_ = true;
    consumer_.notify_all();
    producer_.notify_all();
}

template <class T>
void BlockQueue<T>::clear() {
    lock_guard<mutex> locker(mut_);
    deq_.clear();
}

template <class T>
bool BlockQueue<T>::empty() {
    lock_guard<mutex> locker(mut_);
    return deq_.empty();
}

template <class T>
bool BlockQueue<T>::full() {
    lock_guard<mutex> locker(mut_);
    return deq_.size() >= capacity_;
}

template <class T>
void BlockQueue<T>::push_back(const T& item) {
    unique_lock<mutex> locker(mut_);
    while (deq_.size() >= capacity_) {
        producer_.wait(locker);
    }
    deq_.push_back(item);
    consumer_.notify_one();
}

template <class T>
void BlockQueue<T>::push_front(const T& item) {
    unique_lock<mutex> locker(mut_);
    while (deq_.size() >= capacity_) {
        producer_.wait(locker);
    }
    deq_.push_front(item);
    consumer_.notify_one();
}

template <class T>
bool BlockQueue<T>::pop(T& item) {
    unique_lock<mutex> locker(mut_);
    while (deq_.empty()) {
        consumer_.wait(locker);
    }
    item = deq_.front();
    deq_.pop_front();
    producer_.notify_one();
    return true;
}

template <class T>
bool BlockQueue<T>::pop(T &item,int timeout) {
    unique_lock<mutex> locker(mut_);
    while (deq_.empty()) {
        if (consumer_.wait_for(locker,chrono::seconds(timeout))
            == cv_status::timeout) {
            return false;
        }
        if (isClose_) {
            return false;
        }
    }
    item = deq_.front();
    deq_.pop_front();
    producer_.notify_one();
    return true;
}

template <class T>
T BlockQueue<T>:: front() {
    lock_guard<mutex> locker(mut_);
    return deq_.front();
}

template <class T>
T BlockQueue<T>::back() {
    lock_guard<mutex> locker(mut_);
    return deq_.back();
}

template <class T>
size_t BlockQueue<T>::capacity() {
    lock_guard<mutex> locker(mut_);
    return capacity_;
}

template <class T>
size_t BlockQueue<T>::size() {
    lock_guard<mutex> locker(mut_);
    return deq_.size();
}

template <class T>
void BlockQueue<T>::flush() {
    consumer_.notify_one();
}
