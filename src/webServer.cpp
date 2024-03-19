#include "webServer.h"

WebServer::WebServer(int port,int trigMode,int timeoutMs,bool openLinger,
              int sqlPort,const char* sqlUser,const char* sqlPassword,
              const char* dbName,int connectionPollNumber,int threadNumber,
              bool openLog,int logLevel,int logQueSize):
              port_(port), openLinger_(openLinger), timeoutMs_(timeoutMs), 
              isClose_(false), timer_(new TimerManage()), threadPool_(new ThreadPool(threadNumber)), epoller_(new Epoller()) {
    if (openLog) {
        Log::instance()->init(logLevel,"",".log",logQueSize);
        if (isClose_) {
            LOG_DEBUG("Server init error");
        } else {
            LOG_INFO("server init successful!");
        }
    }
    srcDir_ = getcwd(nullptr,0);
    assert(srcDir_ != nullptr);
    strcat(srcDir_,"/resources/");
    HttpConnection::userCount = 0;
    HttpConnection::srcDir = srcDir_;

    SqlConnectionPool::instance()->init("localhost",sqlPort,sqlUser,sqlPassword,dbName,connectionPollNumber);
    initEventMode_(trigMode);
    if (!initSocket_()) {
        isClose_ = true;
    }

    
}

WebServer::~WebServer() {
    close(listenFd_);
    isClose_ = true;
    free(srcDir_);
    SqlConnectionPool::instance()->closePool();
}

void WebServer::initEventMode_(int trigMode) {
    listenEvent_ = EPOLLRDHUP;
    connectionEvent_ = EPOLLONESHOT | EPOLLHUP;
    switch (trigMode) {
    case 0:
        break;
    case 1:
        connectionEvent_ |= EPOLLET;
        break;
    case 2:
        listenEvent_ |= EPOLLET;
        break;
    case 3:
        listenEvent_ |= EPOLLET;
        connectionEvent_ |= EPOLLET;
        break;
    default:
        listenEvent_ |= EPOLLET;
        connectionEvent_ |= EPOLLET;
    }
    HttpConnection::isET = (connectionEvent_ & EPOLLET);
}

void WebServer::start() {
    int timeMs = -1;
    if (!isClose_) {
        LOG_INFO("Server start");
    }
    while (!isClose_) {
        if (timeoutMs_ > 0) {
            timeMs = timer_->getNextTick();
        }
        int eventCnt = epoller_->wait(timeMs);
        for (int i = 0;i < eventCnt;i ++) {
            int fd = epoller_->getEventFd(i);
            uint32_t events = epoller_->getEvents(i);
            if (fd == listenFd_) {
                dealListen_();
            } else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(users_.count(fd) > 0);
                closeConnetcion_(&users_[fd]);
            } else if (events & EPOLLIN) {
                assert(users_.count(fd) > 0);
                dealRead_(&users_[fd]);
            } else if (events & EPOLLOUT) {
                assert(users_.count(fd) > 0);
                dealWrite_(&users_[fd]);
            } else {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}

void WebServer::sendError_(int fd,const char* info) {
    assert(fd > 0);
    int ret = send(fd,info,strlen(info),0);
    if (ret < 0) {
        LOG_WARN("send error to client[%d] error!",fd);
    }
    close(fd);
}

void WebServer::closeConnetcion_(HttpConnection* client) {
    assert(client != nullptr);
    LOG_INFO("Client[%d] quit!",client->getFd());
    epoller_->delFd(client->getFd());
    client->Close();
}

void WebServer::addClient_(int fd,sockaddr_in addr) {
    assert(fd > 0);
    users_[fd].init(fd,addr);
    if (timeoutMs_ > 0) {
        timer_->add(fd,timeoutMs_,std::bind(&WebServer::closeConnetcion_,this,&users_[fd]));
    }
    epoller_->addFd(fd,EPOLLIN | connectionEvent_);
    setFdNonBlocking(fd);
    LOG_INFO("Client[%d] in!",users_[fd].getFd());
}

void WebServer::dealListen_() {
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do {
        int fd = accept(listenFd_,(sockaddr*)&addr,&len);
        if (fd <= 0) {
            return;
        } else if (HttpConnection::userCount >= MAX_FD) {
            sendError_(fd,"Server busy!");
            LOG_WARN("Client is full!");
            return;
        }
        addClient_(fd,addr);
    } while (listenEvent_ & EPOLLET);
}

void WebServer::dealRead_(HttpConnection* client) {
    assert(client != nullptr);
    extentTime_(client);
    threadPool_->AddTask(std::bind(&WebServer::onRead_,this,client));
}

void WebServer::dealWrite_(HttpConnection* client) {
    assert(client != nullptr);
    extentTime_(client);
    threadPool_->AddTask(std::bind(&WebServer::onWrite_,this,client));
}

void WebServer::extentTime_(HttpConnection* client) {
    assert(client != nullptr);
    if (timeoutMs_ > 0) {
        timer_->adjust(client->getFd(),timeoutMs_);
    }
}

void WebServer::onRead_(HttpConnection* client) {
    assert(client);
    int ret = -1,readErrno = 0;
    ret = client->read(&readErrno);
    if (ret <= 0 && readErrno != EAGAIN) {
        closeConnetcion_(client);
        return;
    }
    onProcess_(client);
}

void WebServer::onProcess_(HttpConnection* client) {
    if (client->process()) {
        epoller_->modFd(client->getFd(),connectionEvent_ | EPOLLOUT);
    } else {
        epoller_->modFd(client->getFd(),connectionEvent_ | EPOLLIN);
    }
}

void WebServer::onWrite_(HttpConnection* client) {
    assert(client != nullptr);
    int ret = -1,writeErrno = 0;
    ret = client->write(&writeErrno);
    if (client->toWriteBytes() == 0) {
        if (client->isAlive()) {
            epoller_->modFd(client->getFd(),connectionEvent_ | EPOLLIN);
            return;
        }
    } else if (ret < 0) {
        if (writeErrno == EAGAIN) {
            epoller_->modFd(client->getFd(),connectionEvent_ | EPOLLOUT);
            return;
        }
    }
    closeConnetcion_(client);
}

bool WebServer::initSocket_() {
    int ret;
    sockaddr_in addr;
    if (port_ > 65535 || port_ < 1024) {
        LOG_ERROR("Port:%d error!",port_);
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);
    {
        linger optLinger;
        if (openLinger_) {
            optLinger.l_linger = 1;
            optLinger.l_onoff = 1;
        }

        listenFd_ = socket(AF_INET,SOCK_STREAM,0);
        if (listenFd_ < 0) {
            LOG_ERROR("Create socket Error!",port_);
            return false;
        }
        ret = setsockopt(listenFd_,SOL_SOCKET,SO_LINGER,&optLinger,sizeof(optLinger));
        if (ret < 0) {
            close(listenFd_);
            LOG_ERROR("init linger error!",port_);
            return false;
        }
    }
    int optval = 1;
    ret = setsockopt(listenFd_,SOL_SOCKET,SO_REUSEADDR,(const void*)&optval,sizeof(int));
    if (ret == -1) {
        LOG_ERROR("set socket setsockopt error!");
        close(listenFd_);
        return false;
    }

    ret = bind(listenFd_,(sockaddr*)&addr,sizeof(addr));
    if (ret < 0) {
        LOG_ERROR("Bind Port:%d error!",port_);
        close(listenFd_);
        return false;
    }
    ret = listen(listenFd_,6);
    if (ret < 0) {
        LOG_ERROR("Listen port:%d Error!",port_);
        close(listenFd_);
        return false;
    }
    ret = epoller_->addFd(listenFd_,listenEvent_ | EPOLLIN);
    if (ret == 0) {
        close(listenFd_);
        return false;
    }
    setFdNonBlocking(listenFd_);
    LOG_INFO("Server port:%d",port_);
    return true;
}

int WebServer::setFdNonBlocking(int fd) {
    assert(fd > 0);
    return fcntl(fd,F_SETFL,fcntl(fd,F_GETFD,0) | O_NONBLOCK);
}