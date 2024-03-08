#ifndef BUFFER_H
#define BUFFER_H
#include <cstring>
#include <iostream>
#include <io.h>
#include <vector>
#include <atomic>
#include <cassert>

class Buffer {    
public:
    Buffer(int ininBufferSize = 1024);
    ~Buffer() = default;

    size_t WritableBytes()      const;
    size_t ReadableBytes()      const;
    size_t PrependableBytes()   const;

    const char* Peek()          const;
    void EnsureWritable(size_t len);
    void HasWritten(size_t len);

    void Retrieve(size_t len);
    void RetrieveUntil(const char* end);
    void RetrieveAll();
    std::string RetrieveAllToStr();

    const char* BeginWriteConst() const;
    char* BeginWrite();

    void Append(const std::string& str);
    void Append(const char* str,size_t len);
    void Append(const void* data,size_t len);
    void Append(const Buffer& buffer);

    auto ReadFd(int fd,int* Errno);
    auto WriteFd(int fd,int* Errno);
    
private:
    char* BeginPtr(); // Buffer开头
    const char* BeginPtr_() const;
    void MakeSpace_(size_t len);

    std::vector<char> buffer_;
    std::atomic<std::size_t> readPos_; // 读下标
    std::atomic<std::size_t> writePos_; // 写下标
};

#endif