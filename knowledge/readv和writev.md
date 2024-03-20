### readv和writev

fd:全称是file descriptor,是进程独有的文件描述符表的索引



在实际开发中，高性能服务有一条减少系统调用的原则。对于一个文件描述符的read或者write，都是系统调用。当我们碰到通过一个文件描述对应的文件或套接字上将数据读到多个缓冲区中去，或者将多个缓冲区中的数据同时写入一个文件描述符对应的文件或者套接字中。你可以调用多次read或者write，挨个操作每个缓冲区，但是这是非常低效的解决方式。

UNIX提供了另外两个函数—readv()和writev()，它们只需一次系统调用就可以实现在文件和进程的多个缓冲区之间传送数据，免除了多次系统调用或复制数据的开销。

readv()称为散布读，即将文件中若干连续的数据块读入内存分散的缓冲区中。writev()称为聚集写，即收集内存中分散的若干缓冲区中的数据写至文件的连续区域中。



```C++
#include <sys/uio.h>

ssize_t readv(int fd, const struct iovec *iov, int iovcnt);
ssize_t writev(int fd, const struct iovec *iov, int iovcnt);
```

iov是一个结构数组，它的每个元素指明存储器中的一个缓冲区。结构类型iovec有下述成员，分别给出缓冲区的起始地址和字节数

```C++
struct iovec {
    void   *iov_base   /* 数据区的起始地址 */
    size_t  iov_len     /* 数据区的大小 */
}
```



readv()和writev()都是原子操作。