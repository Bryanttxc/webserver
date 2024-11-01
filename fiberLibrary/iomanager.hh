#pragma once

#include <error.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>

#include "scheduler.hh"
#include "../util/locker.hh"
#include "../log/logger.hh"
#include "../timer/timer.hh"

namespace bryant{

// 协程 + IO
// 主要解决
class IOManager : public Scheduler, public TimerManager {
public:
    using ptr = std::shared_ptr<IOManager>;

    /**
     * @brief 事件枚举类型
     * 
     */
    enum Event{
        NONE = 0x0,
        READ = 0x1, // EPOLLIN
        WRITE = 0x4 // EPOLLOUT
    };

    /**
     * @brief Construct a new IOManager object
     * 
     * @param num_thread 线程数
     * @param use_caller 是否将主线程也作为调度线程
     * @param name 调度器名
     */
    IOManager(int num_thread = 1, bool use_caller = true, const std::string& name = "IOManager");
    
    /**
     * @brief Destroy the IOManager object
     * 
     */
    ~IOManager();

    /**
     * @brief resize m_fdContexts
     * 
     * @param size 
     */
    void contextResize(size_t size);

    /**
     * @brief add Event
     * 
     * @param fd 句柄
     * @param event 事件类型
     * @param cb 事件对应的回调函数
     * @return int 0->success, -1->fail 
     */
    int addEvent(int fd, Event event, std::function<void()> cb);

    /**
     * @brief delete Event
     * 
     * @param fd 
     * @param event 
     * @return true success
     * @return false fail
     */
    bool delEvent(int fd, Event event);

    /**
     * @brief cancel Event
     * 
     * @param fd 
     * @param event 
     * @return true 
     * @return false 
     */
    bool cancelEvent(int fd, Event event);

    /**
     * @brief cancel all events
     * 
     * @param fd 
     * @param event 
     * @return true 
     * @return false 
     */
    bool cancelAll(int fd);

    /**
     * @brief Get the This IOManager
     * 
     * @return IOManager* 
     */
    static IOManager* GetThis();

protected:
    /**
     * @brief tickle idle threads to execute fiber/cb
     * 
     */
    void tickle() override;

    /**
     * @brief use epoll to wait events
     * 
     */
    void idle() override;

    /**
     * @brief 
     * 
     * @return true 
     * @return false 
     */
    bool stopping() override;
    bool stopping(uint64_t& timeout);

    /**
     * @brief notify epoll_wait to change timeout
     * 
     */
    void onTimerInsertedAtFront() override;

private:
    /**
    * @brief fd上下文类，一个fd下可能包含多个事件，为简化仅分成读和写事件
    * @details 每个socket fd都对应⼀个FdContext，包括fd的值， fd上的事件，以及fd的读写事件上下⽂
    */
    struct FdContexts{
        /**
         * @brief 事件上下文类
         * @details  fd的每个事件都有⼀个事件上下⽂，保存这个事件的回调函数以及执⾏回调函数的调度器
         */
        struct EventContext{
            std::function<void()> cb; // 回调函数
            Fiber::ptr fiber; // 事件协程
            Scheduler* scheduler; // 执行事件的调度器
        };

        /**
         * @brief check the Event Context if read or write
         * 
         */
        EventContext& getEventContext(Event event);

        /**
         * @brief reset Event Context
         * 
         */
        void resetEventContext(EventContext& eventContext);

        /**
         * @brief trigger Event
         * @details 根据事件类型调⽤对应上下⽂结构中的调度器去调度回调协程或回调函数
         */
        void triggerEvent(Event event);

        int fd = 0;             // 描述符
        EventContext read;      // 读事件
        EventContext write;     // 写事件
        Event events = NONE;    // 事件集合
        Mutex mutex;            // 互斥锁
    };

private:
    int m_tickleFds[2];         // 用于通知IO线程的管道
    int m_epollfd;              // epoll文件描述符
    int m_max_events = 256;      // epoll监听event数

    std::atomic<size_t> m_pendingEventCount = {0}; // 等待执行的事件数量
    std::vector<FdContexts*> m_fdContexts; // fd上下文数组
    RWMutex m_rwMutex; // 读写锁
};


}