#include "epoller.h"
#include "stdio.h"
Epoller::Epoller(int maxEvent): epollFd_(epoll_create(512)), events_(maxEvent) {
    assert(epollFd_ >= 0 && events_.size() >= 0);
}

Epoller::~Epoller() {
    close(epollFd_);
}

bool Epoller::addFd(int fd,uint32_t events) {
    if (fd < 0) {
        return false;
    }
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = events;
    return epoll_ctl(epollFd_,EPOLL_CTL_ADD,fd,&ev) == 0;
}

bool Epoller::modFd(int fd,uint32_t events) {
    if (fd < 0) {
        return false;
    }
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = events;
    return epoll_ctl(epollFd_,EPOLL_CTL_MOD,fd,&ev) == 0;
}

bool Epoller::delFd(int fd) {
    if (fd < 0) {
        return false;
    }
    return epoll_ctl(epollFd_,EPOLL_CTL_DEL,fd,0) == 0;
}

// 返回事件个数
int Epoller::wait(int timeoutMs) {
    return epoll_wait(epollFd_,&events_[0],static_cast<int>(events_.size()),timeoutMs);
}

// 获取事件的id
int Epoller::getEventFd(size_t i) const {
    assert(i < events_.size() && i >= 0);
    return events_[i].data.fd;
}

// 获取事件属性
uint32_t Epoller::getEvents(size_t i) const {
    assert(i < events_.size() && i >= 0);
    return events_[i].events;
}