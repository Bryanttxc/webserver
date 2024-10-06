#pragma once

#include <ucontext.h>
#include <memory>
#include <functional>

#include "../log/logger.hh"

namespace bryant{

// 分配栈空间类
class StackAllocator{
public:
    static void* alloca(size_t size){
        return malloc(size);
    }

    static void dealloca(void* stack, size_t size){
        free(stack);
    }
};

class Scheduler;

// 协程
class Fiber : public std::enable_shared_from_this<Fiber>{

friend class Scheduler;

public:
    using ptr = std::shared_ptr<Fiber>;

    enum State{
        READY = 0, //就绪态
        RUNNING, // 运行态
        TERM //结束态
    };

    /**
     * @brief create mission fiber
     * 
     * @param cb callback function
     * @param stack_size stack size of fiber
     */
    Fiber(std::function<void()> cb, int stack_size = 0, bool run_in_scheduler = true);

    /**
     * @brief 销毁协程
     * 
     */
    ~Fiber();
    
    /**
     * @brief 将当前协程切换为任务执行状态，即当前协程与线程主协程切换
     * 当前协程 -> running, 线程主协程 -> ready
     * 
     */
    void resume();

    /**
     * @brief 将当前协程切换为等待状态，即线程主协程与当前协程切换
     * 当前协程 -> ready, 线程主协程 -> running
     * 
     */
    void yield();

    /**
     * @brief 重置协程，包括状态变为ready，栈空间重置大小，上下文重置绑定cb
     * 
     */
    void reset(std::function<void()> cb);

    /**
     * @brief 将当前协程设置为t_fiber 
     * 
     */
    static void SetThis(Fiber* fiber);

    /**
     * @brief 获取当前协程
     * 
     * @return Fiber::ptr 
     */
    static Fiber::ptr GetThis();

    /**
     * @brief 获取当前协程id
     * 
     * @return int 
     */
     int getId() const {return m_id;}

     /**
     * @brief 获取当前协程状态
     * 
     * @return State 
     */
     State getState() const {return m_state;}

private:
    /**
     * @brief 创建主协程
     * 
     */
    Fiber(); 
    
    /**
     * @brief 协程运行主函数
     * 
     * @return void 
     */
    static void mainFun();

private:
    int m_id;                   // 协程id
    State m_state;              // 协程状态
    int m_stack_size;           // 栈大小
    void* m_stack;              // 栈地址
    std::function<void()> m_cb; // 回调函数
    ucontext_t m_ctx;           // 上下文对象
    bool m_run_in_scheduler;    // 是否受调度协程控制
};

}