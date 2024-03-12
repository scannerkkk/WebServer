# WebServer
这是一个基于C++11实现的Reactor模式的WebServer服务器，在Ubuntu20.04环境下编写。

一.设计应用层缓冲区
fd全称是file descriptor,是进程独有的文件描述符表的索引
struct iovec定义了一个向量元素。通常，这个结构用作一个多元素的数组。对于每一个传输的元素，指针成员iov_base指向一个缓冲区，这个缓冲区是存放的是readv所接收的数据或是writev将要发送的数据。成员iov_len在各种情况下分别确定了接收的最大长度以及实际写入的长度。

readv和writev函数用于在一次函数调用中读、写多个非连续缓冲区。有时也将这两个函数称为散布读（scatter read）和聚集写（gather write）

二.设计日志
同步日志和异步日志
同步日志：日志写入函数与工作线程串行执行，由于涉及到I/O操作，当单条日志较大时会阻塞整个处理流程，所以服务器的并发处理能力下降。
异步日志：将所有的日志内容先存到阻塞队列中，写线程从阻塞队列中取出内容，写入日志。是一个经典的生产者消费者模型。

mutex
std::mutex不允许拷贝构造，也不允许move拷贝，最初生产的mutex是处于unlocked状态的

unique_lock和lock_guard

1.unique_lock 和 lock_guard都能实现自动加锁和解锁，但是unique_lock允许中途解锁，lock_guard只能等对象被析构解锁

2.条件变量的目的就是为了，在没有获得某种提醒时长时间休眠，如果正常情况下我们需要一直循环，这样的问题是CPU消耗+时延问题，条件变量的wait一直休眠到执行notify

当场景有notify+wait的场景必须使用unique_lock，如果是单纯的互斥则使用lock_guard，因为条件变量在wait时会进入unlock再进入休眠。

notify_one 和 notify_all

1.notify_one 和 notify_all常用来唤醒阻塞的线程
2.notify_one() 因为只唤醒等待队列中的第一个线程；不存在锁争用，所以能够立即获得锁，其余线程不会被唤醒，需要等待再次调用notify_one或者notify_all()

3.notify_all() 会唤醒所有等待队列中阻塞的线程，存在锁争用，只有一个线程能够获得锁。其他未获取锁的线程会轮询，不会再次阻塞。当持有锁的线程释放锁时，这些线程中的一个会获得锁。其余的会接着尝试获得锁。

wait和wait_for

1.当 std::condition_variable对象的某个wait 函数被调用的时候，它使用 std::unique_lock(通过std::mutex) 来锁住当前线程。
当前线程会一直被堵塞。直到另外一个线程在同样的 std::condition_variable 对象上调用了 notification 函数来唤醒当前线程。

2.与std::condition_variable::wait() 相似，只是 wait_for能够指定一个时间段，在当前线程收到通知或者指定的时间 rel_time 超时之前。该线程都会处于堵塞状态。而一旦超时或者收到了其它线程的通知，wait_for返回，剩下的处理步骤和 wait()相似。

另外，wait_for 的重载版本号的最后一个參数pred表示 wait_for的预測条件。仅仅有当 pred条件为false时调用 wait()才会堵塞当前线程，而且在收到其它线程的通知后仅仅有当 pred为 true时才会被解除堵塞。

可变参数的宏里的’##’操作说明带有可变参数的宏
如果可变参数被忽略或为空，’##’操作将使预处理器(preprocessor)去除掉它前面的那个逗号。

__VA_ARGS__ 是一个可变参数的宏

日志类采用单例模式的懒汉模式
懒汉模式：只有当调用getInstance的时候，才会去初始化这个单列。在C++11后，不需要加锁，直接使用函数内局部函数即可。
饿汉模式：在程序运行时立即初始化。饿汉模式不需要加锁就可以实现线程安全。

snprintf()函数原型为int snprintf(char* str,size_t size,const char* format,...)。;
如果格式化后的字符串长度< size,则将此字符串全部复制到str中，并给其后添加一个字符串结束符
如果格式化后的字符串长度>=size,则只将其中的(size-1)个字符复制到str中，并给其后添加一个字符串结束符，返回值为将要写入的字符串长度（不是实际写入的）

va_start()和va_end() 
这两个都是宏
va_start(list,param)获取可变参数的第一个参数的地址，param是可变参数最左边的参数
va_end()清空va_list可变参数列表

vsnprintf()
它可以根据一个格式字符串和可变数量的参数，将格式化的数据写入一个字符串中。返回错误值或者写入目标字符串的字符数量。
这个函数可以接受一个va_list参数.

三.线程池和连接池
线程池：是一种利用池化技术思想来实现的线程管理技术，主要是为了复用线程，便利地管理线程和任务，并将线程地创建和任务地执行解耦开来。使用线程池来复用已经创建地线程来降低频繁创建和销毁线程所带来地消耗，同时也提高了响应速度。

RALL机制
资源获取即初始化，利用C++构造对象最终被析构函数销毁地原则，在其构造时获取对应的资源，在对象生命期内控制对资源的访问，最后在对象析构的时候，释放构造的资源。

连接池：负责分配，管理和释放数据库连接，它允许应用程序重复使用一个现有的数据库连接，而不是再重新建立一个。
第一次访问的时候，需要建立连接。但是之后的访问，均会复用之前创建的连接，直接执行sql语句。
优点：减少了网络开销、系统的性能得到了提升。

具有代表性的两款产品：
HikariCP
Druid

使用make_shared代替new + shared复制，它可以在单一的内存分配中同时分配对象的存储空间和引用计数控制块，并减少了引用计数的内存开销。

explicit只能用于类内部得函数定义

detach()函数
称为分离线程函数，使用detach()函数会让线程在后台运行，即说明主线程不会等待子线程运行结束才结束。
通常称为分离线程为守护线程，UNIX中守护线程是指，没有任何显式的用户接口，并在后台运行的线程。这种线程的特点就是长时间运行。
弊端是：可能main线程终止了，但是子线程还没有终止。

sem_t
信号量是一种变量，其访问是原子操作的。这就使得应用程序可以对其采用两种操作。
1、等待信号量，当信号量是0的时候，程序阻塞等待。当信号量 > 0 的时候，程序就可以继续运行了。
2、发送信号量，其作用是让信号量的值+1。这就实现了线程的同步控制。

四.HTTP状态机
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
+       url中+号表示空格
空格    url中空格可以为+号或者%20
%       指定特殊字符
#       表示书签
&       URL中指定的参数间的分隔符
=       URL中指定参数的值
?       用于分隔路径和查询参数等

stat函数用来获取指定路径的文件或者文件夹的信息
成功返回0，否则返回-1


struct stat 
{
    dev_t     st_dev;         /* ID of device containing file */文件使用的设备号
    ino_t     st_ino;         /* Inode number */ 索引节点号
    mode_t    st_mode;        /* File type and mode */文件类型和模式
    nlink_t   st_nlink;       /* Number of hard links */文件的硬连接数
    uid_t     st_uid;         /* User ID of owner */所有者的用户ID
    gid_t     st_gid;         /* Group ID of owner */所有者的组ID
    dev_t     st_rdev;        /* Device ID (if special file) */设备ID
    off_t     st_size;        /* Total size, in bytes */以字节为单位的文件容量
    blksize_t st_blksize;     /* Block size for filesystem I/O */文件系统 I/O块大小
    blkcnt_t  st_blocks;      /* Number of 512B blocks allocated */文件所占的磁盘块

    /* 自 Linux 2.6 起，内核支持纳秒以下时间戳字段的精度。
                    Linux 2.6 之前的详细信息，请参见注释。 */

    struct timespec st_atim;  /* Time of last access*/最后一次访问该文件的时间
    struct timespec st_mtim;  /* Time of last modification*/最后一次修改该文件的时间
    struct timespec st_ctim;  /* Time of last status change*/最后一次修改文件状态的时 
    间

    #define st_atime st_atim.tv_sec      /* 向后兼容 */
    #define st_mtime st_mtim.tv_sec
    #define st_ctime st_ctim.tv_sec
    };