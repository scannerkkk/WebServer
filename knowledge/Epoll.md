### epoll和select和poll

操作系统在处理IO的时候，主要有两个阶段：

1.等待数据传输到IO设备（等到数据传到kernel内核space）

2.IO设备将数据复制到user space（kernel内核区将数据复制到user space）



select、poll、epoll都是IO多路复用的机制。

IO多路复用就是通过一种机制，可以监视多个描述符，一但某个描述符就绪，能够通知程序进行相应的读写操作。但是select、poll、epoll本质上都是同步IO，因为他们都需要在读写事件就绪后自己负责进行读写，也就是说这个读写过程是阻塞的，而异步IO则无需自己负责进行读写，异步IO的实现会把数据从内核拷贝到用户空间。



select

单个进程就可以同时处理多个网络连接的IO请求（同时阻塞多个IO操作），基本原理就是程序呼叫select，然后整个程序就是阻塞状态，这时候，kernel内核就会轮询检查所有select负责的文件描述符fd，当找到其中那个的数据准备好了的fd，就会返回给select，select通知系统调用，将数据从kernel内核复制到进程缓冲区（用户空间）



select缺点

1.每次调用select，都需要把fd集合从用户态拷贝到内核态，这个开销在fd很多时会很大。

2.同时每次调用select都需要在内核遍历传递进来的所有fd，这个开销在fd很多时也很大。

3。select支持的文件描述符数量太少了，默认是1024。



poll

poll的原理和select非常相似，差别如下：

1.描述fd集合的方式不同，poll使用pollfd结构而不是select结构fd_set，所以poll是链式的，没有最大连接数的限制。

2.poll有一个特点是水平触发，也就是通知程序fd就绪后，这次没有被处理，那么下次poll的时候会再次通知同个fd已经就绪。



epoll

epoll提供了三个函数：

1.int epoll_create(int size);建立一个epoll对象，并传回它的id

2.int epoll_ctl(int epfd,int op,int fd,struct epoll_event* event);事件注册函数，将需要监听的事件和需要监听的fd交给epoll对象。

3.int epoll_wait(int epfd,struct epoll_event* events,int maxevents,int timeout);等待注册的事件被触发或者timeout发生。



1.epoll没有fd数量限制epoll没有这个限制，我们知道每个epoll监听一个fd，所以最大数量与能打开的fd数量有关，一个g的内存的机器上，能打开10万个左右。

2.epoll不需要每次都从用户空间将fd_set复制到kernel epoll，在用epoll_ctl函数进行注册的时候，已经将fd复制到内核中，所以不需要每次都重新复制一次。

3.select和poll都是主动轮询机制，需要遍历每一个人的fd；epoll是被动触发方式，给fd注册了相应事件后，我们为每一个fd指定了一个回调函数，当数据准备好之后，就会把就绪的fd加入一个就绪的队列中，epoll_wait的工作方式实际上就是在这个就绪队列中查看有没有就绪的fd，如果有，就唤醒就绪队列上的等待者，然后调用回调函数。

events可以是以下几个宏的集合：
EPOLLIN ：表示对应的文件描述符可以读（包括对端SOCKET正常关闭）；
EPOLLOUT：表示对应的文件描述符可以写；
EPOLLPRI：表示对应的文件描述符有紧急的数据可读（这里应该表示有带外数据到来）；
EPOLLERR：表示对应的文件描述符发生错误；
EPOLLHUP：表示对应的文件描述符被挂断；
EPOLLET： 将EPOLL设为边缘触发(Edge Triggered)模式，这是相对于水平触发(Level Triggered)来说的。
EPOLLONESHOT：只监听一次事件，当监听完这次事件之后，如果还需要继续监听这个socket的话，需要再次把这个socket加入到EPOLL队列里



ET和LT两种工作模式：

epoll有两种工作模式LT（水平触发）模式和ET（边缘触发）模式。

默认情况下，epoll采用LT模式工作，这时候可以处理阻塞和非阻塞套接字，而上表中的EPOLLET表示可以将一个事件改为ET模式，ET模式效率要比LT模式高，但是只适用于非阻塞套接字。

ET模式仅当状态发生变化的时候才获得通知,这里所谓的状态的变化并不包括缓冲区中还有未处理的数据,也就是说,如果要采用ET模式,需要一直read/write直到出错为止,很多人反映为什么采用ET模式只接收了一部分数据就再也得不到通知了,大多因为这样;而LT模式是只要有数据没有处理就会一直通知下去的.