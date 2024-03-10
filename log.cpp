#include "log.h"

Log::Log() {
    fp_ = nullptr;
    Q_ = nullptr;
    writeThread_ = nullptr;
    lineCount_ = 0;
    toDay_ = 0;
    isAsync_ = false;
}

Log::~Log() {
    while (!Q_->empty()) {
        Q_->flush();                // 唤醒消费者，处理掉剩下的任务
    }
    Q_->close();                    // 关闭队列
    writeThread_->join();           // 等待当前线程完成手中的任务
    if (fp_) {                      // 关闭文件描述符
        lock_guard<mutex> locker(mut_); 
        flush();                    // 清空缓冲区中的数据
        fclose(fp_);                // 关闭日志文件
    }
}

// 唤醒阻塞队列消费者，开始写日志
void Log::flush() {
    if (isAsync_) {
        Q_->flush();
    }
    fflush(fp_);
}

// 懒汉模式 
Log* Log::instance() {
    static Log log;
    return &log;
}

// 异步日志的写线程
void Log::flushLogThread() {
    Log::instance()->asyncWrite_();
}

// 写线程的执行函数
void Log::asyncWrite_() {
    string str("");
    while (Q_->pop(str)) {
        lock_guard<mutex> locker(mutex);
        fputs(str.c_str(),fp_);
    }
}

// 初始化日志实例
void Log::init(int level,const char* path,const char* suffix,int maxCapacity){
    level_ = level;
    isOpen_ = true;
    path_ = path;
    suffix_ = suffix;
    if (maxCapacity) { // 异步
        isAsync_ = true;
        if (Q_ == nullptr) { // 为空则创建
            unique_ptr<BlockQueue<std::string>> nQ(new BlockQueue<std::string>);
            Q_ = move(nQ);
            unique_ptr<thread> nT(new thread(flushLogThread));
            writeThread_ = move(nT);
        }
    } else {
        isAsync_ = false;
    }
    lineCount_ = 0;
    time_t timer = time(nullptr);
    tm* systime = localtime(&timer);
    char fileName[LOG_NAME_LEN] = {0};
    snprintf(fileName,LOG_NAME_LEN - 1,"%s%04d_%02d_%02d%s",path_,systime->tm_year + 1900,systime->tm_mon + 1,systime->tm_mday,suffix_);
    toDay_ = systime->tm_mday;
    {
        lock_guard<mutex> locker(mut_);
        buffer_.RetrieveAll();
        if (fp_) {
            flush();
            fclose(fp_);
        }
        fp_ = fopen(fileName,"a");
        if (fp_ == nullptr) { // 目录不存在则创建
            mkdir(path_,0777);
            fp_ = fopen(fileName,"a");
        }
        assert(fp_ != nullptr);
    }
}

void Log::write(int level,const char* format,...) {
    timeval now = {0,0};
    gettimeofday(&now,nullptr);
    time_t tSec = now.tv_sec;
    tm* sysTime = localtime(&tSec);
    tm t = *sysTime;
    va_list vaList;

    if (toDay_ != t.tm_mday || (lineCount_ && (lineCount_ % MAX_LINES == 0))) {
        unique_lock<mutex> locker(mut_);
        locker.unlock();
        char newFile[LOG_NAME_LEN];
        char tail[36] = {0};
        snprintf(tail,36,"%04d_%02d_%02d",t.tm_year + 1900,t.tm_mon + 1,t.tm_mday);
        if (toDay_ != t.tm_mday) {
            snprintf(newFile,LOG_NAME_LEN - 72,"%s/%s%s",path_,tail,suffix_);
        } else {
            snprintf(newFile,LOG_NAME_LEN - 72,"%s/%s-%d%s",path_,tail,(lineCount_ / MAX_LINES),suffix_);
        }
        locker.lock();
        flush();
        fclose(fp_);
        fp_ = fopen(newFile,"a");
        assert(fp_ != nullptr);
    }

    {
        unique_lock<mutex> locker(mut_);
        lineCount_ ++;
        int n = snprintf(buffer_.BeginWrite(),128,"%d-%02d-%02d %02d:%02d:%02d.%06ld",
                        t.tm_year + 1900,t.tm_mon + 1,t.tm_mday,t.tm_hour,t.tm_min,t.tm_sec,now.tv_usec);
        buffer_.HasWritten(n);
        appendLogLevelTitle_(level);
        va_start(vaList,format);
        int m = vsnprintf(buffer_.BeginWrite(),buffer_.WritableBytes(),format,vaList);
        va_end(vaList);
        buffer_.HasWritten(m);
        buffer_.Append("\n\0",2);

        if (isAsync_ && Q_ != nullptr && !Q_->full()) { // 异步放入阻塞队列
            Q_->push_back(buffer_.RetrieveAllToStr());
        } else { // 同步写入
            fputs(buffer_.Peek(),fp_);
        }
        buffer_.RetrieveAll(); // 清空buffer
    }
}

// 添加日志等级
void Log::appendLogLevelTitle_(int level) {
    switch (level)
    {
    case 0:
        buffer_.Append("[debug]:",9);
        break;
    case 1:
        buffer_.Append("[info]:",9);
        break;
    case 2:
        buffer_.Append("[warn]:",9);
        break;
    case 3:
        buffer_.Append("[error]:",9);
        break;
    default:
        buffer_.Append("[info]:",9);
        break;
    }
}

int Log::getLevel() {
    lock_guard<mutex> locker(mut_);
    return level_;
}

void Log::setLevel(int level) {
    lock_guard<mutex> locker(mut_);
    level_ = level;
}

bool Log::isOpen() {
    return isOpen_;
}