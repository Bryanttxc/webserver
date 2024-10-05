#pragma once

#include <atomic>
#include <functional>
#include <pthread.h> 
#include <memory>

#include "../util/locker.hh"

namespace bryant{

// 自定义线程类
// 一共有两种线程，要么是系统创建的主线程，要么是Thread创建的线程
class Thread{
public:
    using ptr = std::shared_ptr<Thread>; // 线程智能指针

    Thread(std::function<void()> cb, const std::string& name);
    ~Thread();

    pid_t getId() const {return m_id;}
    const std::string& getName() const {return m_name;}

    /**
     * @brief thread join
     * 
     */
    void join();

public:
    /**
     * @brief Get the current Thread Id（only one -> static）
     * 
     * @return pid_t 
     */
    static pid_t GetThreadId();

    /**
     * @brief Get the current Thread（only one -> static）
     *        for logger
     * @return Thread* 
     */
    static Thread* GetThis();

    /**
     * @brief Get the current Thread name（only one -> static）
     *        for logger
     * @return const std::string& 
     */
    static const std::string& GetName();

    /**
     * @brief Set the Name object
     * 
     * @param name 
     */
    static void SetName(const std::string& name);

private:
    // forbid copy and move
    Thread(const Thread&) = delete;
    Thread(const Thread&&) = delete;
    Thread& operator=(const Thread&) = delete;

    /**
     * @brief thread callback
     * 
     * @param arg 
     * @return void* 
     */
    static void* run(void* arg);

private:
    pid_t m_id = -1;            // 线程id
    pthread_t m_thread = 0;     // 线程句柄

    std::string m_name;         // 线程名
    std::function<void()> m_cb; // 回调函数

    Semaphore m_sem;            // 信号量
};


}