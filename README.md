# WebServer

## log
日志系统，负责同步/异步记录日志信息
> * 日志系统采用单例模式
> * 异步日志系统采用阻塞队列实现，数据结构采用循环数组

## fiberLibrary
### thread 线程
> * 封装pthread类，增加信号量控制
### fiber 协程
> * 采用非对称、独立栈的形式构建协程
### scheduler N-M协程调度器
> * 采用N-M协程调度器，实现协程的异步运行
> * 可将主线程作为调度线程
### iomanager 协程IO调度器
> * 在scheduler的基础上，用epoll解决idle协程忙询占用CPU问题
> * epoll事件支持添加、删除、取消

## timer
定时器相关类
### timer
> * 定时器类，包含取消，重置等功能
> * 需要通过TimerManager类来创建
### TimerManager
> * 定时器管理类，包含增删改等功能
> * 采用小根堆的方式，保证定时器按顺序执行

## util
工具类，包含一些常用的工具函数
> * locker.hh
> * 互斥锁Mutex, 信号量Semaphore, 读写锁RWMutex

## test
测试文件夹，包含对每个模块的测试
### test_locker.cc
> * 针对互斥锁Mutex设计抢票场景
> * 针对信号量Semaphore设计生产者-消费者场景
### test_logger.cc
> * 测试Logger类成员函数执行是否正确
> * 测试BlockQueue类运行是否正常
### test_thread.cc
> * 测试Thread类成员函数执行是否正确
### test_fiber.cc
> * 配合Thread类测试Fiber类成员函数
> * 根据日志查看Fiber的异步运行顺序
### test_scheduler.cc
> * 测试use_caller=true时，单线程-Fiber的异步运行顺序
> * 测试use_caller=false时，多线程-Fiber的异步运行顺序
### test_iomanager.cc
> * 待完善
### test_timer.cc
> * 测试TimerManager的功能是否正常