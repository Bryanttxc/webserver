#pragma once

#include <functional>
#include <list>
#include <memory>
#include <vector>
#include <string>

#include "fiber.hh"
#include "thread.hh"
#include "../util/locker.hh"
#include "../util/util.hh"

namespace bryant{

/**
 * @brief 简单协程调度类，支持添加调度任务以及运行调度任务
 */
class Scheduler{
public:
    using ptr = std::shared_ptr<Scheduler>;

    /**
     * @brief Construct a new Scheduler object
     * 
     * @param num_thread 线程数
     * @param use_caller 是否将主线程也作为调度线程
     * @param name 调度器名字
     */
    Scheduler(int num_thread = 1, bool use_caller = true, const std::string& name = "Scheduler");

    /**
     * @brief Destroy the Scheduler object
     * 
     */
    virtual ~Scheduler();

    /**
     * @brief start scheduler
     * 
     */
    void start();

    /**
     * @brief stop schedule
     * 
     * @return true 
     * @return false 
     */
    void stop();

    /**
     * @brief Get the scheduler name
     * 
     * @return const std::string& 
     */
    const std::string& getName() const {return m_name;}

    /**
     * @brief Get the Scheduler
     * 
     * @return Scheduler::ptr 
     */
    static Scheduler* GetThis();

    /**
     * @brief Get the Main Fiber object
     * 
     * @return Fiber::ptr 
     */
    static Fiber* GetMainFiber();

protected:
    /**
     * @brief notify thread to execute fiber/cb
     * 
     */
    virtual void tickle();

    /**
     * @brief run scheduler
     * 
     */
    void run();

    /**
    * @brief execute idle when no mission
    */
    virtual void idle();

    /**
    * @brief return whether if can stop
    */
    virtual bool stopping();

    /**
    * @brief set this object as scheduler
    */
    static void SetThis(Scheduler* scheduler);

    /**
    * @brief return if has idle threadas
    * @details 当调度协程进⼊idle时空闲线程数加1，从idle协程返回时空闲线程数减1
    */
    bool hasIdleThreads() {return m_idleThreadCount > 0;}

private:
    /**
    * @brief 调度任务类，协程/函数二选一，可指定在哪个线程上调度
    */
    struct ScheduleTask{
        std::function<void()> cb; // 函数
        Fiber::ptr fiber; // 协程 是否要加全局"::"
        int thread; // 指定线程执行

        // 重载函数，保证对于下列输入都能构造Task
        ScheduleTask(std::function<void()> f, int thr)
            :cb(f), thread(thr){
        }

        // 传递的是指向智能指针的指针，用swap可以避免引用释放相关的问题
        ScheduleTask(std::function<void()>* f, int thr)
            :thread(thr){
            cb.swap(*f);
        }

        ScheduleTask(Fiber::ptr f, int thr)
            :fiber(f), thread(thr){
        }

        // 传递的是指向智能指针的指针，用swap可以避免引用释放相关的问题
        ScheduleTask(Fiber::ptr* f, int thr)
            :thread(thr){
            fiber.swap(*f);
        }

        ScheduleTask()
            :thread(-1){
        }

        /**
         * @brief 重置任务
         * 
         */
        void reset(){
            cb = nullptr;
            fiber = nullptr;
            thread = -1;
        }
    };

public:
    /**
     * @brief 添加一个调度任务，并通知线程处理
     * 
     * @tparam FiberOrCb 
     * @param cb 
     * @param thread 
     */
    template <typename FiberOrCb>
    void schedule(FiberOrCb cb, int thread = -1){
        bool need_tickle = !m_tasks.empty();

        m_mutex.lock();
        need_tickle = scheduleNoLock(cb, thread);
        m_mutex.unlock();

        // 通知线程处理
        if(need_tickle){
            tickle();
        }
    }

    /**
     * @brief 添加多个调度任务
     * 
     * @tparam InputIterator 
     * @param input 
     */
    template <typename InputIterator>
    void schedule(InputIterator begin, InputIterator end, int thread = -1){
        bool need_tickle = !m_tasks.empty();
        
        m_mutex.lock();
        while(begin != end){
            need_tickle = scheduleNoLock(&*begin, thread) || need_tickle;
            ++begin;
        }
        m_mutex.unlock();

        // 通知线程处理
        if(need_tickle){
            tickle();
        }
    }

private:
    /**
     * @brief 往m_tasks添加调度任务
     * 
     * @tparam FiberOrCb 
     * @param cb 函数/协程
     * @param thread 执行线程
     */
    template <typename FiberOrCb>
    bool scheduleNoLock(FiberOrCb cb, int thread){
        ScheduleTask task(cb, thread); // 创建一个任务
        if(task.fiber || task.cb){
            m_tasks.push_back(task);
            return true;
        }

        return false;
    }

private:
    std::string m_name; // 调度器名

    int m_threadNum;    // 线程数
    std::vector<Thread::ptr> m_thrs;    // 线程池数组
    std::list<ScheduleTask> m_tasks;    // 任务队列

    std::vector<int> m_threads_id; // 线程id数组
    std::atomic<size_t> m_activeThreadCount = {0};  // 活跃线程数
    std::atomic<size_t> m_idleThreadCount = {0};    // 空闲线程数

    bool m_use_caller; // 是否将主线程也作为调度线程
    Fiber::ptr m_rootFiber; // use_caller为true时，主线程的调度线程
    int m_rootThread = 0; // 主线程id

    Mutex m_mutex; // 互斥锁

    bool m_stopping = false; // 是否停止调度
};


}

