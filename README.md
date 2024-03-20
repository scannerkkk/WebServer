# WebServer
这是一个基于C++11/14实现的Reactor模式的WebServer服务器，在Ubuntu22.04环境下编写。



### 设计日志

同步日志和异步日志
同步日志：日志写入函数与工作线程串行执行，由于涉及到I/O操作，当单条日志较大时会阻塞整个处理流程，所以服务器的并发处理能力下降。
异步日志：将所有的日志内容先存到阻塞队列中，写线程从阻塞队列中取出内容，写入日志。是一个经典的生产者消费者模型。



### VA_ARGS

可变参数的宏里的’##’操作说明带有可变参数的宏
如果可变参数被忽略或为空，’##’操作将使预处理器(preprocessor)去除掉它前面的那个逗号。

VA_ARGS 是一个可变参数的宏

va_start()和va_end() 
这两个都是宏
va_start(list,param)获取可变参数的第一个参数的地址，param是可变参数最左边的参数
va_end()清空va_list可变参数列表

vsnprintf()
它可以根据一个格式字符串和可变数量的参数，将格式化的数据写入一个字符串中。返回错误值或者写入目标字符串的字符数量。
这个函数可以接受一个va_list参数.

### HTTP状态机

HTTP实现功能：
1.读取请求
2.解析请求
3.生成响应
4.发送响应

从解析请求行到解析请求头再到解析请求体，是一个自动的过程

利用正则表达式解析HTTP报文
$表示从尾端开始
^表示除这个字符之外
regex_match 全文匹配，符合返回true，否则返回false

mysql_query
字符串必须包含1条sql语句，而且不应为语句添加中介分号';'或‘/g’。如果允许多语句执行，字符串可包含多条由分号隔开的语句。但是连接的时候必须指定CLIENT_MULTI_STATEMENTS选项。

mysql_query()不能用于包含二进制数据的查询，应使用mysql_real_query()取而代之。

查询成功返回0，否则返回非0

url编码解码

url中+号表示空格
空格    url中空格可以为+号或者%20
\%       指定特殊字符
\#       表示书签
&       URL中指定的参数间的分隔符
=       URL中指定参数的值
?       用于分隔路径和查询参数等

stat函数用来获取指定路径的文件或者文件夹的信息
成功返回0，否则返回-1

open函数用于打开和创建文件，成功返回fd，否则返回-1。
O_RDONLY 只读模式
O_WRONLY 只写模式
O_RDWR   读写模式



sockaddr在头文件#include<sys/socket.h>中定义，sockaddr的缺陷是：sa_data把目标地址和端口信息混在一起了。

sockaddr_in
sockaddr_in在头文件#include<netinet/in.h>或#include <arpa/inet.h>中定义，该结构体解决了sockaddr的缺陷，把port和addr 分开储存在两个变量中。

inet_ntoa函数
inet_ntoa函数所在的头文件：<arpa/inet.h>
将一个网络字节序的IP地址（也就是结构体in_addr类型变量）转化为点分十进制的IP地址（字符串）。

errno定义于头文件<cerrno> <errno.h> errno 是用于错误指示的预处理器宏

c++11为了减少平台差异匹配了POSIX错误码中的大多数具体标准c的请参见<errno.h>文件及包含部分，有些可能是平台扩展的linux可以参照 man errno

std::chrono::high_resolution_clock
duration：一段时间
time_point：时间点

high_resolution_clock::now()方法，返回当前时间，返回得时间点是按秒为单位得。
std::chrono::milliseconds表示毫秒，可用于duration<>的模板类

### Epoll

为什么IO复用需要用非阻塞IO

原因1：当数据到达socket缓冲区得时候，select会报告这个socket可读，但是随后因为一些原因，比如校验和错误，内核丢弃了这个数据，这个时候，如果采用了阻塞IO，唤醒得程序去读取一个已经被丢弃得数据，肯定读不到，所以就会一直阻塞到那里。

原因2：到达缓冲区得数据有可能被别人抢走，比如多个进程accept同一个socket时引发得惊群现象，只有一个客户端连接到来，但是所有得监听程序都会被唤醒，最终只有一个进程可以accept到这个请求。

原因3：ET边缘模式下，必须要用到非阻塞得IO，因为程序中需要循环读和写，直到EAGAIN得出现，如果使用阻塞IO就容易被阻塞住。

EAGAIN是一个错误代码
如果socket的状态为非阻塞，但是accept函数没有找到可用的连接，就会返回EAGAIN错误。

应用1：accept
我们时常会在死循环中设置if(errno == EAGAIN) break;一旦accept在缓冲区找不到可用的连接了，那么accept会将errno设置为EAGAIN，这个时候这个判断就说明，accept已经将缓冲区中的连接读取完了，所以break

应用2：recv和send
前提：非阻塞的IO、EPOLL ET边缘触发模式

recv：在EPOLLIN|EPOLLET监视可读、边缘触发模式下，recv函数会时刻关注缓冲区中是否有数据可读，如果缓冲区中有数据未处理，EPOLLET模式下的epoll只会汇报一次socket有可读事件，当有新的数据加入缓冲区时，就会再次汇报可读。

send：在EPOLLOUT|EPOLLET监视可读、边缘触发模式下，send函数会时刻关注缓冲区中是否已满，如果发送缓冲区未满，EPOLLET模式下就会触发一次可写事件，只有当缓冲区从满变为“有空”的时候，才会再次触发可写事件。
