#ifndef HTTP_CONNECTION_H
#define HTTP_CONNECTION_H

#include <sys/types.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include "log.h"
#include "buffer.h"
#include "httpRequest.h"
#include "httpResponse.h"

class HttpConnection {
public:
    HttpConnection();
    ~HttpConnection();
    void init(int socketFd,const sockaddr_in& address);
    ssize_t read(int* saveErrno);
    ssize_t write(int* saveErrno);
    void Close();
    int getFd()             const;
    int getPort()           const;
    const char* getIp()     const;
    sockaddr_in getAddr()   const;
    bool process();

    int toWriteBytes();
    bool isAlive();

    static bool isET;
    static const char* srcDir;
    static std::atomic<int> userCount;

private:
    int fd_;
    sockaddr_in addr_;
    bool isClose_;
    int iovCnt_;
    iovec iov_[2];

    Buffer readBuffer_;
    Buffer writeBuffer_;

    HttpRequest request_;
    HttpResponse response_;
};
#endif