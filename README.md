# WebServer
这是一个基于C++11实现的Reactor模式的WebServer服务器，在Ubuntu20.04环境下编写。

一.
fd全称是file descriptor,是进程独有的文件描述符表的索引
struct iovec定义了一个向量元素。通常，这个结构用作一个多元素的数组。对于每一个传输的元素，指针成员iov_base指向一个缓冲区，这个缓冲区是存放的是readv所接收的数据或是writev将要发送的数据。成员iov_len在各种情况下分别确定了接收的最大长度以及实际写入的长度。

readv和writev函数用于在一次函数调用中读、写多个非连续缓冲区。有时也将这两个函数称为散布读（scatter read）和聚集写（gather write）

二.
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

另外，wait_for 的重载版本号的最后一个參数pred表示 wait_for的预測条件。仅仅有当 pred条件为false时调用 wait()才会堵塞当前线程，而且在收到其它线程的通知后仅仅有当 pred为 true时才会被解除堵塞，因此相当于例如以下代码：