#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <cassert>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "epoller.cpp"
#include "timerManage.cpp"
#include "log.cpp"
#include "sqlConnectionPool.cpp"
#include "threadPool.h"
#include "httpConnection.cpp"

class WebServer {
public:
    WebServer(int port,int trigMode,int timeoutMs,bool openLinger,
              int sqlPort,const char* sqlUser,const char* sqlPassword,
              const char* dbName,int connectionPollNumber,int threadNumber,
              bool openLog,int logLevel,int logQueSize);
    
    ~WebServer();
    void start();

private:
    bool initSocket_();
    void initEventMode_(int trigMode);
    void addClient_(int fd,sockaddr_in addr);

    void dealListen_();
    void dealWrite_(HttpConnection* client);
    void dealRead_(HttpConnection* client);
    void sendError_(int fd,const char* info);
    void extentTime_(HttpConnection* client);
    void closeConnetcion_(HttpConnection* client);

    void onRead_(HttpConnection* client);
    void onWrite_(HttpConnection* client);
    void onProcess_(HttpConnection* client);

    static const int MAX_FD = 65535;
    static int setFdNonBlocking(int fd);

    int port_;
    bool openLinger_;
    int timeoutMs_; // 毫秒
    bool isClose_;
    int listenFd_;
    char* srcDir_;

    uint32_t listenEvent_; // 监听事件
    uint32_t connectionEvent_; // 连接事件

    std::unique_ptr<TimerManage> timer_;
    std::unique_ptr<ThreadPool> threadPool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int,HttpConnection> users_;
};
#endif