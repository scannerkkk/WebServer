#include "httpConnection.h"

const char* HttpConnection::srcDir;
atomic<int>HttpConnection::userCount;
bool HttpConnection::isET;

bool HttpConnection::isAlive() {
    return request_.isAlive();
}

int HttpConnection::toWriteBytes() {
    return iov_[0].iov_len + iov_[1].iov_len;
}

HttpConnection::HttpConnection() {
    fd_ = -1;
    addr_ = {0};
    isClose_ = true;
}

HttpConnection::~HttpConnection() {
    Close();
}

void HttpConnection::init(int fd,const sockaddr_in& addr) {
    assert(fd > 0);
    userCount ++;
    addr_ = addr;
    fd_ = fd;
    writeBuffer_.RetrieveAll();
    readBuffer_.RetrieveAll();
    isClose_ = false;
    LOG_INFO("Client[%d](%s:%d) login, userCount:%d",fd_,getIp(),getPort(),(int)userCount);
}

void HttpConnection::Close() {
    response_.unmapFile();
    if (isClose_ == false) {
        isClose_ = true;
        userCount --;
        close(fd_);
        LOG_INFO("Client[%d](%s:%d) quit, userCount:%d",fd_,getIp(),getPort(),(int)userCount);
    }
}

int HttpConnection::getFd() const {
    return fd_;
}

sockaddr_in HttpConnection::getAddr() const {
    return addr_;
}

const char* HttpConnection::getIp() const {
    return inet_ntoa(addr_.sin_addr);
}

int HttpConnection::getPort() const {
    return addr_.sin_port;
}

ssize_t HttpConnection::read(int* saveErrno) {
    ssize_t len = -1;
    do {
        len = readBuffer_.ReadFd(fd_,saveErrno);
        if (len <= 0) {
            break;
        }
    } while (isET);
    return len;
}

ssize_t HttpConnection::write(int* saveErrno) {
    ssize_t len = -1;
    do {
        len = writev(fd_,iov_,iovCnt_);
        if (len <= 0) {
            *saveErrno = errno;
            break;
        }
        if (iov_[0].iov_len + iov_[1].iov_len == 0) {
            break;
        } else if (static_cast<size_t>(len) > iov_[0].iov_len) {
            iov_[1].iov_base = (uint8_t*)iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if (iov_[0].iov_len) {
                writeBuffer_.RetrieveAll();
                iov_[0].iov_len = 0;
            }
        } else {
            iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len;
            iov_[0].iov_len -= len;
            writeBuffer_.Retrieve(len);
        }
    } while (isET || toWriteBytes() > 10240);
    return len;
}

bool HttpConnection::process() {
    request_.init();
    if (readBuffer_.ReadableBytes() <= 0) {
        return false;
    } else if (request_.parse(readBuffer_)) {
        LOG_DEBUG("%s",request_.path().c_str());
        response_.init(srcDir,request_.path(),request_.isAlive(),200);
    } else {
        response_.init(srcDir,request_.path(),false,400);
    }
    response_.makeResponse(writeBuffer_);

    iov_[0].iov_base = const_cast<char*>(writeBuffer_.Peek());
    iov_[0].iov_len = writeBuffer_.ReadableBytes();
    iovCnt_ = 1;

    if (response_.fileLen() > 0 && response_.file()) {
        iov_[1].iov_base = response_.file();
        iov_[1].iov_len = response_.fileLen();
        iovCnt_ = 2;
    }
    LOG_DEBUG("filesize:%d, %d to %d",response_.fileLen(),iovCnt_,toWriteBytes());
    return true;
}