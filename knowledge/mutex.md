### mutex互斥量

包括在#include<mutex>头文件中

C++11提供四种语义的互斥量

1.std::mutex，独占的互斥量，不能递归使用

2.std::time_mutex，带超时的独占互斥量，不能递归使用

3.std::recursive_mutex，递归互斥量，不带超时功能

4.std::recursive_timed_mutex，带超时的递归互斥量



std::mutex的成员函数

1.mutex不允许拷贝构造，也不允许move拷贝，最初产生的mutex对象是处于unlocked的。

2.lock()，调用线程将锁住该互斥量。

3.unlock()，解锁，释放对互斥量的所有权。

4.try_lock()，尝试锁住互斥量，如果互斥量被其他线程占有，则当前线程也不会被阻塞。



lock_guard和unique_lock的区别

1.unique_lock与lock_guard都能实现自动加锁和解锁，但是前者更加灵活。

2.unique_lock可以进行临时解锁再上锁，而不必等到析构时自动解锁，lock_guard是不支持手动释放的。

3.一般来说，使用unique_lock比较多，除非追求极致的性能才会考虑使用lock_guard。



### 条件变量

condition_variable

在C++11中，我们可以使用条件变量试实现多个线程间的同步操作，当条件不满足时，相关线程会被一直阻塞，直到某种条件出现，这些线程才会被唤醒。



为了防止竞争，条件变量的使用总是和一个互斥锁结合在一起；通常情况下这个锁是std::mutex，并且管理这个锁 只能是 std::unique_lock  <std::mutex> RAII模板类



1.等待条件成立使用的是condition_variable类成员wait 、wait_for 或 wait_until。

2.给出信号使用的是condition_variable类成员notify_one或者notify_all函数。



wait和wait_for

wait 导致当前线程阻塞直至条件变量被**通知**，或**虚假唤醒发生**，可选地循环直至满足某谓词。

wait_for 导致当前线程阻塞直至条件变量被**通知**，或**虚假唤醒发生**，或者**超时返回**。

虚假唤醒：

在正常情况下，wait类型函数返回时要不是因为被唤醒，要不是因为超时才返回，但是在实际中发现，因此操作系统的原因，wait类型在不满足条件时，它也会返回，这就导致了虚假唤醒



notify_all和notify_one

1.notify_one 和 notify_all常用来唤醒阻塞的线程

2.notify_one() 因为只唤醒等待队列中的第一个线程；不存在锁争用，所以能够立即获得锁，其余线程不会被唤醒，需要等待再次调用notify_one或者notify_all()

3.notify_all() 会唤醒所有等待队列中阻塞的线程，存在锁争用，只有一个线程能够获得锁。其他未获取锁的线程会轮询，不会再次阻塞。当持有锁的线程释放锁时，这些线程中的一个会获得锁。其余的会接着尝试获得锁。