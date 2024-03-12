#include "sqlConnectionPool.h"

SqlConnectionPool* SqlConnectionPool::instance() {
    static SqlConnectionPool pool;
    return &pool;
}

void SqlConnectionPool::init(const char* host,int port,const char* user,const char* password,
                             const char* dbName,int connectionSize = 10) {
    assert(connectionSize > 0);
    for (int i = 0;i < connectionSize;i ++) {
        MYSQL* connection = mysql_init(connection);
        if (connection == nullptr) {
            LOG_ERROR("MySql init Error!");
            assert(connection);
        }
        connection = mysql_real_connect(connection,host,user,password,dbName,port,nullptr,0);
        if (connection == nullptr) {
            LOG_ERROR("MySql Connect Error!");
        }
        Q_.emplace(connection);
        MAX_CONNECTION_ = connectionSize;
        sem_init(&semId_,0,MAX_CONNECTION_);
    }
}

MYSQL* SqlConnectionPool::getConnection() {
    MYSQL* connection = nullptr;
    if (Q_.empty()) {
        LOG_WARN("SqlConnectionPool busy!");
        return nullptr;
    }
    sem_wait(&semId_);
    lock_guard<mutex> locker(mut_);
    connection = Q_.front();
    Q_.pop();
    return connection;
}

// 放入连接池，并没有关闭连接
void SqlConnectionPool::freeConnection(MYSQL* connection) {
    assert(connection != nullptr);
    lock_guard<mutex> locker(mut_);
    Q_.push(connection);
    sem_post(&semId_);
}

int SqlConnectionPool::getFreeConnectionCount() {
    lock_guard<mutex> locker(mut_);
    return Q_.size();
}

// 释放所有连接
void SqlConnectionPool::closePool() {
    lock_guard<mutex> locker(mut_);
    while (!Q_.empty()) {
        auto connection = Q_.front();
        Q_.pop();
        mysql_close(connection);
    }
    mysql_library_end();
}