# WebServer

## log
日志系统，负责同步/异步记录日志信息
> * 日志系统采用单例模式
> * 异步日志系统采用阻塞队列实现，数据结构采用循环数组

## util
工具类，包含一些常用的工具函数
> * locker.hh
> * 互斥锁Mutex, 信号量Semaphore

## test
测试文件夹，包含对每个模块的测试
### test_locker.cc
> * 针对互斥锁Mutex设计抢票场景
> * 针对信号量Semaphore设计生产者-消费者场景
### test_logger.cc
> * 测试Logger类成员函数执行是否正确
> * 测试BlockQueue类运行是否正常
