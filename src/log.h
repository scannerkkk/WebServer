#ifndef LOG_H
#define LOG_H
#include <mutex>
#include <string>
#include <thread>
#include <sys/time.h>
#include <cstring>
#include <cstdarg>
#include <cassert>
#include <sys/stat.h>
#include "blockQueue.cpp"
#include "buffer.cpp"

class Log {
public:
    void init(int level,const char* path = "./log",const char* suffix = ".log",int maxCapacity = 1024);
    
    static Log* instance();
    static void flushLogThread();

    void write(int level,const char* format,...);
    void flush();

    int getLevel();
    void setLevel(int);
    bool isOpen();

private:
    Log();
    void appendLogLevelTitle_(int level);
    virtual ~Log();
    void asyncWrite_();

private:
    static const int LOG_PATH_LEN = 256;
    static const int LOG_NAME_LEN = 256;
    static const int MAX_LINES = 100000;

    const char* path_;
    const char* suffix_;

    int maxLines_;
    int lineCount_;
    int toDay_;
    bool isOpen_;
    Buffer buffer_;
    int level_;
    bool isAsync_;

    FILE* fp_;
    std::unique_ptr<BlockQueue<std::string>> Q_;
    std::unique_ptr<std::thread> writeThread_;
    std::mutex mut_;
};

#define LOG_BASE(level,format,...) \
    do { \
        Log* log = Log::instance(); \
        if (log->isOpen() && log->getLevel() <= level) { \
            log->write(level,format,## __VA_ARGS__); \
            log->flush(); \
        } \
    } while (0); 

#define LOG_DEBUG(format,...) do {LOG_BASE(0,format,## __VA_ARGS__)} while (0);
#define LOG_INFO(format,...) do {LOG_BASE(1,format,## __VA_ARGS__)} while (0);
#define LOG_WARN(format,...) do {LOG_BASE(2,format,## __VA_ARGS__)} while (0);
#define LOG_ERROR(format,...) do {LOG_BASE(3,format,## __VA_ARGS__)} while (0);

#endif