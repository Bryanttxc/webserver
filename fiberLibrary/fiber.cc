#include <cassert>
#include <atomic>

#include "fiber.hh"
#include "scheduler.hh"

namespace bryant{

std::atomic<int> s_fiber_id(0); // 协程id生成器
std::atomic<int> s_fiber_count(0); // 协程数

static thread_local Fiber* t_fiber = nullptr; // 正在运行的协程(主协程/任务协程)
static thread_local Fiber::ptr t_thread_fiber = nullptr; // 线程主协程


void 
Fiber::SetThis(Fiber* fiber){
    t_fiber = fiber;
}


Fiber::ptr 
Fiber::GetThis(){
    if(t_fiber){
        return t_fiber->shared_from_this();
    }
    Fiber::ptr mainFiber(new Fiber());
    t_fiber = mainFiber.get();
    t_thread_fiber = mainFiber;

    return t_fiber->shared_from_this();
}


Fiber::Fiber(std::function<void()> cb, int stack_size, bool run_in_scheduler):
    m_cb(std::move(cb)), 
    m_state(READY),
    m_run_in_scheduler(run_in_scheduler){

    ++s_fiber_count;
    m_id = s_fiber_id;
    ++s_fiber_id;

    // 栈空间
    m_stack_size = stack_size ? stack_size : 1024*1024;
    m_stack = StackAllocator::alloca(m_stack_size);
    
    // 对m_ctx进行初始化
    // ps: 一开始未加getcontext导致core dumpeds
    if(getcontext(&m_ctx)) {
        throw std::logic_error("getcontext error");
    }
    m_ctx.uc_link = nullptr; // 设置上下文链
    m_ctx.uc_stack.ss_sp = m_stack; // 分配栈空间
    m_ctx.uc_stack.ss_size = m_stack_size; // 设置栈大小
    
    makecontext(&m_ctx, &Fiber::mainFun, 0); // 将m_ctx与m_cb绑定

    LOG_DEBUG("[Fiber] Thread %lu Mission Fiber %lu created", bryant::GetThreadId(), m_id);
}


Fiber::Fiber(){
    SetThis(this); // 因为是主协程
    m_state = RUNNING;
    m_stack = nullptr; // 防止未初始化内存导致的非预期结果

    ++s_fiber_count;
    m_id = s_fiber_id;
    ++s_fiber_id;

    assert(getcontext(&m_ctx) == 0);

    LOG_DEBUG("[Fiber] Thread %lu Main Fiber %lu created", bryant::GetThreadId(), m_id);
}


Fiber::~Fiber(){
    --s_fiber_count;
    if(m_stack){ // 栈空间仍存在 (任务协程有分配)
        assert(m_state == TERM);
        StackAllocator::dealloca(m_stack, m_stack_size); // 释放栈资源
        m_stack_size = 0;
        m_stack = nullptr;
    } else { // 栈空间不存在 (主协程无分配)
        assert(!m_cb);
        assert(m_state == RUNNING);

        if(t_fiber == this){ // 如果t_fiber还是当前协程，那么设置为null
            SetThis(nullptr);
        }
    }

    LOG_DEBUG("[Fiber] Thread %lu Fiber %lu destroyed", bryant::GetThreadId(), m_id);
}


void 
Fiber::resume(){
    assert(m_state != TERM && m_state != RUNNING);
    SetThis(this);
    m_state = RUNNING;

    LOG_DEBUG("[Fiber] Thread %lu Fiber %lu resume: enter swapcontext", bryant::GetThreadId(), m_id);
    if(m_run_in_scheduler){
        // 和调度器的调度协程交换上下文
        assert(swapcontext(&(Scheduler::GetMainFiber()->m_ctx), &m_ctx) == 0);
    } else {
        assert(swapcontext(&(t_thread_fiber->m_ctx), &m_ctx) == 0);
    }
    LOG_DEBUG("[Fiber] Thread %lu Fiber %lu resume: quit swapcontext", bryant::GetThreadId(), m_id);
}


void 
Fiber::yield(){
    assert(m_state == RUNNING || m_state == TERM);
    SetThis(t_thread_fiber.get());
    if(m_state != TERM){
        m_state = READY; // 之前错写成RUNNING
    }

    LOG_DEBUG("[Fiber] Thread %lu Fiber %lu yield: enter swapcontext", bryant::GetThreadId(), m_id);
    if(m_run_in_scheduler){
        // 和调度器的调度协程交换上下文
        assert(swapcontext(&m_ctx, &(Scheduler::GetMainFiber()->m_ctx)) == 0);
    } else {
        assert(swapcontext(&m_ctx, &(t_thread_fiber->m_ctx)) == 0);
    }
    LOG_DEBUG("[Fiber] Thread %lu Fiber %lu yield: quit swapcontext", bryant::GetThreadId(), m_id);
}


void
Fiber::reset(std::function<void()> cb){
    assert(m_stack);
    assert(m_state == TERM);
    
    m_cb = std::move(cb);
    
    if(getcontext(&m_ctx)) {
        throw std::logic_error("getcontext error");
    }
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stack_size;

    makecontext(&m_ctx, &Fiber::mainFun, 0);
    m_state = READY;
}


void 
Fiber::mainFun(){
    Fiber::ptr fiber = GetThis();
    assert(fiber != nullptr);

    fiber->m_cb();
    fiber->m_cb = nullptr;
    fiber->m_state = TERM;

    auto raw_ptr = fiber.get(); // 手动让t_fiber引用计数减1
    fiber.reset();
    raw_ptr->yield(); // 自动切换为yield状态
}


}