#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h>
#include <unistd.h>
#include <cassert>
#include <vector>
#include <errno.h>
class Epoller {
public:
    explicit Epoller(int maxEvent = 1024);
    ~Epoller();

    bool addFd(int fd,uint32_t events);
    bool modFd(int fd,uint32_t events);
    bool delFd(int fd);
    int wait(int timeoutMs = -1);
    uint32_t getEvents(size_t i) const;
    int getEventFd(size_t i) const;

private:
    int epollFd_;
    std::vector<struct epoll_event> events_;
};
#endif