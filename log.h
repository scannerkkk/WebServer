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
    // 初始化日志实例，（阻塞队列最大容量，日志保存路径，日志文件后缀）
    void init(int level,const char* path = "./log",const char* suffix = ".log",int maxCapacity = 1024);
    
    static Log* instance();
    static void flushLogThread(); // 异步写日志公有方法，调用私有方法asyncWrite

    void write(int level,const char* format,...);
    void flush();

    int getLevel();
    void setLevel(int);
    bool isOpen();

private:
    Log();
    void appendLogLevelTitle_(int level);
    virtual ~Log();
    void asyncWrite_(); // 异步写日志方法

private:
    static const int LOG_PATH_LEN = 256; // 日志文件最长文件名
    static const int LOG_NAME_LEN = 256; // 日志最长名字
    static const int MAX_LINES = 100000; // 日志文件内的最长日志条数

    const char* path_;      // 路径名
    const char* suffix_;    // 后缀名

    int maxLines_;          // 最大日志行数
    int lineCount_;         // 日志行数记录
    int toDay_;             // 按当天日期区分文件
    bool isOpen_;           // 
    Buffer buffer_;         // 输出的内容，缓冲区
    int level_;             // 日志等级
    bool isAsync_;          // 是否开启异步日志

    FILE* fp_;                                          // 打开log的文件指针
    std::unique_ptr<BlockQueue<std::string>> Q_;        // 阻塞队列
    std::unique_ptr<std::thread> writeThread_;          // 写线程的指针
    std::mutex mut_;                                    // 同步日志的互斥量
};

#define LOG_BASE(level,format,...) \
    do { \
        Log* log = Log::instance(); \
        if (log->isOpen() && log->getLevel() <= level) { \
            log->write(level,format,##__VA_ARGS_); \
            log->flush(); \
        } \
    } while (0); 

#define LOG_DEBUG(format,...) do {LOG_BASE(0,format,##__VA_ARGS_)} while (0);
#define LOG_INFO(format,...) do {LOG_BASE(1,format,##__VA_ARGS_)} while (0);
#define LOG_WARN(format,...) do {LOG_BASE(2,format,##__VA_ARGS_)} while (0);
#define LOG_ERROR(format,...) do {LOG_BASE(3,format,##__VA_ARGS_)} while (0);

#endif