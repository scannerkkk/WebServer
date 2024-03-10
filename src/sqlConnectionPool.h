#ifndef SQL_CONNECTION_POOL_H
#define SQL_CONNECTION_POOL_H

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include "log.cpp"

class SqlConnectionPool {
public:
    static SqlConnectionPool* instance();
    MYSQL* getConnection();
    void freeConnection(MYSQL* connection);
    int getFreeConnectionCount();

    void init(const char* host,int port,const char* user,const char* password,
              const char* dbName,int connectionSize);
    
    void closePool();

private:
    SqlConnectionPool() = default;
    ~SqlConnectionPool() {
        closePool();
    }

    int MAX_CONNECTION_;
    std::queue<MYSQL*> Q_;
    std::mutex mut_;
    sem_t semId_;
};

class SqlConnectionRall {
public:
    SqlConnectionRall(MYSQL** sql,SqlConnectionPool* pool) {
        assert(pool);
        *sql = pool->getConnection();
        sql_ = *sql;
        pool_ = pool;
    }
private:
    MYSQL* sql_;
    SqlConnectionPool* pool_;
};
#endif