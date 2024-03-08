#include "buffer.h"

// 构造函数初始化
Buffer::Buffer(int initBufferSize) :buffer_(initBufferSize), readPos_(0), writePos_(0) {}

// 当前可写的数据大小 = buffer_大小 - 写下标
size_t Buffer::WritableBytes() const {
    return buffer_.size() - writePos_;
}

// 当前可读的数据大小 = 写下标 - 读下标
size_t Buffer::ReadableBytes() const {
    return writePos_ - readPos_;
}

// 可预留空间：已经读过了的 用于内部挪腾
size_t Buffer::PrependableBytes() const {
    return readPos_;
}

// 读下标地址
const char* Buffer::Peek() const {
    return &buffer_[read_Pos];
}

// 确保可写长度
void Buffer::EnsureWritable(size_t len) {
    if (len > WritableBytes()) {
        MakeSpace_(len);
    }
    assert(len <= WritableBytes());
}

// 移动写下标，在Append中使用
void Buffer::HasWritten(size_t len) {
    writePos_ += len;
}

// 读取len长度，移动读下标
void Buffer::Retrieve(size_t len) {
    readPos_ += len;
}

// 读取到end这个位置
void Buffer::RetrieveUntil(const char* end) {
    assert(Peek() <= end);
    Retrieve(end - Peek());
}

// 取出所有数据，buffer归零，读写下标归零，在别的函数中会用到
// bzero函数将内存块（字符串）的前n个字节清零
void Buffer::RetrieveAll() {
    bzero(&buffer_[0],buffer_.size());
    readPos_ = writePos_ = 0;
}

// 取出剩余可读的str
std::string Buffer::RetrieveAllToStr() {
    std::string str(Peek(),ReadableBytes());
    RetrieveAll();
    return str;
}

// 写指针的位置
char* Buffer::BeginWrite() {
    return &buffer_[writePos_];
}

const char* Buffer::BeginWriteConst() const {
    return &buffer_[writePos_];
}

// 添加str到缓冲区
void Buffer::Append(const char* str,size_t len) {
    assert(str);
    EnsureWritable(len);
    std::copy(str,str + len,BeginWrite());
    HasWritten(len);
}

void Buffer::Append(const std::string& str) {
    Append(str.c_str(),str.size());
}

void Buffer::Append(const void* data,size_t len) {
    Append(static_cast<const char*>(data),len);
}

// 将buffer中的读下标的地方放到该buffer中的写下标位置
void Buffer::Append(const Buffer& buffer) {
    Append(buffer.Peek(),buffer.ReadableBytes());
}

// 将fd的内容读到缓冲区，即Writable位置
auto Buffer::ReadFd(int fd,int* Errno) {
    char buffer[65535]; // 临时栈
    struct iovec iov[2];
    
}

