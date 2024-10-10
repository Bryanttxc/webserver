#include <cassert>

#include "hook.hh"
#include "scheduler.hh"

namespace bryant{

// ps: 当use_caller = true时，主线程需要有三类协程：主协程，调度协程，任务协程
// 主协程：创建调度器，开启调度器并添加任务，停止调度器
// 调度协程：负责充当原先线程主协程的角色，管理任务协程
// 任务协程：一个个任务ScheduleTask
static thread_local Scheduler* t_scheduler = nullptr;   // 主线程的主协程
static thread_local Fiber* t_schedule_fiber = nullptr;  // 主线程/子线程的调度协程


void
Scheduler::SetThis(Scheduler* scheduler){
    t_scheduler = scheduler;
}


Scheduler*
Scheduler::GetThis(){
    return t_scheduler;
}


Fiber*
Scheduler::GetMainFiber(){
    return t_schedule_fiber;
}


// 只有在主线程才会调用Scheduler构造函数
Scheduler::Scheduler(int num_thread, bool use_caller, const std::string& name)
    :m_threadNum(num_thread),
     m_use_caller(use_caller),
     m_name(name){

    if(m_threadNum < 0){
        throw std::logic_error("m_threadNum must greater than 0");
    }

    if(m_use_caller){
        --m_threadNum;

        // 创建主线程的主协程
        bryant::Fiber::GetThis();
        
        // 设置主线程为调度器
        assert(GetThis() == nullptr);
        t_scheduler = this;

        // 创建t_schedule_fiber
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, false));
        
        bryant::Thread::SetName(m_name);
        t_schedule_fiber = m_rootFiber.get();
        m_rootThread = bryant::GetThreadId();
        m_threads_id.push_back(m_rootThread);
    } else {
        // 主线程不需要t_schedule_fiber
        m_rootThread = -1;
    }
}


Scheduler::~Scheduler(){
    m_thrs.clear();
    m_threads_id.clear();
    m_tasks.clear();
}


void
Scheduler::start(){
    LOG_DEBUG("[scheduler] Scheduler '%s' start", m_name.c_str());

    m_mutex.lock();

    assert(m_thrs.empty()); // 检查线程池是否为空

    m_thrs.resize(m_threadNum); // 调整线程池大小
    for(int i = 0; i < m_threadNum; ++i){
        m_thrs[i].reset(new Thread(
                    std::bind(&Scheduler::run, this), 
                    m_name + "_" + std::to_string(i)));
        m_threads_id.push_back(m_thrs[i]->getId());
    }

    m_mutex.unlock();
}


void 
Scheduler::stop(){
    LOG_DEBUG("[scheduler] Scheduler '%s' stop", m_name.c_str());
    if(stopping()){
        return;
    }
    m_stopping = true;

    if(m_use_caller){
        assert(GetThis() == this);
    } else {
        assert(GetThis() != this);
    }

    for(size_t i = 0; i < m_threadNum; ++i){
        tickle();
    }

    if(m_rootFiber){
        tickle();
    }

    // 在use_caller = true下，调度器协程结束时，应该返回caller协程
    if(m_rootFiber){
        m_rootFiber->resume(); // yield返回到主线程的主协程
        LOG_DEBUG("[scheduler] m_rootFiber end");
    }

    std::vector<Thread::ptr> thrs;
    {
        m_mutex.lock();
        thrs.swap(m_thrs);
        m_mutex.unlock();
    }
    for(auto& thr : thrs){
        thr->join();
    }
}


// 新线程的入口函数
void
Scheduler::run(){
    SetThis(this);
    set_hook_enable(true); // enable hook
    LOG_DEBUG("[scheduler] Thread %lu t_schedule_fiber %d run", 
                bryant::GetThreadId(), Fiber::GetThis()->getId());

    // 当use_caller = false 或 当前线程不是主线程
    if(bryant::GetThreadId() != m_rootThread){
        t_schedule_fiber = Fiber::GetThis().get(); // 线程主协程
    }

    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this))); // idle协程
    Fiber::ptr cb_fiber; // 任务协程
    ScheduleTask task; // 任务

    while(true){
        // step1 初始化
        task.reset();
        bool tickle_me = false; // 通知其他线程的flag

        // step2 从任务队列里取属于当前线程的任务
        {
            m_mutex.lock();

            auto it = m_tasks.begin();
            while(it != m_tasks.end()){
                // 找到属于其他线程的任务
                if(it->thread != -1 && it->thread != bryant::GetThreadId()){
                    ++it;
                    tickle_me = true;
                    continue;
                }

                // 找到属于未绑定/当前线程的任务
                assert(it->fiber || it->cb);
                if(it->fiber){
                    assert(it->fiber->getState() == Fiber::State::READY);
                }
                task = *it;
                m_tasks.erase(it);
                ++it;
                ++m_activeThreadCount;
                break;
            }

            // 当前线程领完任务后，队列还有剩余，那就通知其他线程来取
            tickle_me |= (it != m_tasks.end());
            m_mutex.unlock();
        }

        if(tickle_me){
            tickle();
        }

        // step3 执行任务
        if(task.fiber){
            task.fiber->resume();
            --m_activeThreadCount;
        }
        else if(task.cb) {
            if(cb_fiber){
                cb_fiber->reset(task.cb); // 用到Fiber::reset
            } else {
                cb_fiber.reset(new Fiber(task.cb));
            }

            task.reset();
            cb_fiber->resume();
            --m_activeThreadCount;
            cb_fiber.reset();
        } else {
            // 说明队列已空，调度idle协程进入idle状态
            if(idle_fiber->getState() == Fiber::State::TERM){
                LOG_DEBUG("[scheduler] idle fiber term");
                break;
            }

            ++m_idleThreadCount;
            idle_fiber->resume();
            --m_activeThreadCount;
        }
    } // while(true)

    // 对于use_caller = true来说，t_schedule_fiber的m_run_in_scheduler = false
    // 因此t_schedule_fiber的resume()里的上下文会和t_thread_fiber交换
    // 这也导致了下面的Fiber::GetThis()返回的是t_thread_fiber的id
    LOG_DEBUG("[scheduler] Thread %lu t_schedule_fiber %d::run() exit", 
                bryant::GetThreadId(), Fiber::GetThis()->getId());
}


bool
Scheduler::stopping(){
    return m_stopping;
}


void
Scheduler::idle(){
    LOG_DEBUG("[scheduler] idle Fiber %lu is running", Fiber::GetThis()->getId());
    sleep(2);
}


void
Scheduler::tickle(){
    LOG_DEBUG("[scheduler] Thread %lu send tickle to other threads", bryant::GetThreadId());
}

}