#include "timer.hh"

namespace bryant{

Timer::Timer(int ms, std::function<void()> cb, 
            TimerManager* timer_manager, bool recurring) 
    :m_ms(ms), 
     m_cb(cb),
     m_manager(timer_manager),
     m_recurring(recurring){
    
    m_next = bryant::GetCurrentMS() + m_ms;
}


Timer::Timer(uint64_t next)
    :m_next(next){
}


bool
Timer::cancel(){
    m_manager->m_rwMutex.writeLock();
    if(m_cb){
        m_cb = nullptr;
        m_manager->m_rwMutex.unlock();
        return true;
    }

    m_manager->m_rwMutex.unlock();
    return false;
}


// refresh 只会向后调整
bool 
Timer::refresh(){
    m_manager->m_rwMutex.writeLock();
    
    if(!m_cb){ // 已执行完
        return false;
    }

    auto it = m_manager->m_timers.find(shared_from_this());
    if(it == m_manager->m_timers.end()){ // m_timers里没找到
        return false;
    }
    m_manager->m_timers.erase(it);
    m_next = bryant::GetCurrentMS() + m_ms;
    m_manager->m_timers.insert(shared_from_this());

    m_manager->m_rwMutex.unlock();
    return true;
}


bool 
Timer::reset(uint64_t ms, bool from_now){
    if(ms == m_next && !from_now){
        return true;
    }
    
    m_manager->m_rwMutex.writeLock();

    if(!m_cb){ // 已经执行完了
        return false;
    }

    auto it = m_manager->m_timers.find(shared_from_this());
    if(it == m_manager->m_timers.end()){ // m_timers里没找到
        return false;
    }
    m_manager->m_timers.erase(it);
    
    int start = 0;
    if(from_now){
        start = bryant::GetCurrentMS();
    } else {
        start = bryant::GetCurrentMS() - m_next;
    }
    m_ms = ms;
    m_next = start + m_ms;
    m_manager->addTimer(shared_from_this(), m_manager->m_rwMutex);

    return true;
}


TimerManager::TimerManager(){
}


TimerManager::~TimerManager(){
}


Timer::ptr 
TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring){
    Timer::ptr timer(new Timer(ms, cb, this, recurring));
    m_rwMutex.writeLock();
    addTimer(timer, m_rwMutex);
    return timer;
}


void
TimerManager::addTimer(Timer::ptr timer, RWMutex& rw_mutex){
    auto it = m_timers.insert(timer).first;
    bool at_front = (it == m_timers.begin());
    
    rw_mutex.unlock();

    // 有更早的事件需要处理，通知一下xx，更改epoll_wait的等待时间
    if(at_front){
        onTimerInsertedAtFront();
    }
}


// 如果条件存在，执行cb()
static void onTimer(std::weak_ptr<void> condition, std::function<void()> cb){
    std::shared_ptr<void> tmp = condition.lock(); // 获取智能指针
    if(tmp){
        cb();
    }
}


Timer::ptr  
TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb, 
                                std::weak_ptr<void> condition, bool recurring){
    return addTimer(ms, std::bind(&onTimer, condition, cb), recurring);
}


uint64_t
TimerManager::getNextTimer(){
    m_rwMutex.readLock();
    if(m_timers.empty()){
        m_rwMutex.unlock();
        return ~0ull; // 最大值
    }

    Timer::ptr timer = *m_timers.begin();
    uint64_t now_ms = bryant::GetCurrentMS();
    if(now_ms >= timer->m_next){
        m_rwMutex.unlock();
        return 0;
    }

    uint64_t left_time = timer->m_next - now_ms;
    m_rwMutex.unlock();

    return left_time;
}


void
TimerManager::listExpiredCb(std::vector<std::function<void()>>& cbs){
    uint64_t now_ms = bryant::GetCurrentMS();
    {
        m_rwMutex.readLock();
        if(m_timers.empty()){
            m_rwMutex.unlock();
            return;
        }
        m_rwMutex.unlock();
    }

    m_rwMutex.writeLock();

    // 通过当前时间创建一个定时器，分出m_timers超时的部分
    Timer::ptr now_timer(new Timer(now_ms));
    auto it = m_timers.lower_bound(now_timer);
    while(it != m_timers.end() && (*it)->m_next == now_ms){
        ++it;
    }

    std::vector<Timer::ptr> expired; // 超时的定时器集合
    expired.insert(expired.begin(), m_timers.begin(), it);
    m_timers.erase(m_timers.begin(), it);
    cbs.resize(expired.size());

    // 处理超时的定时器
    for(auto& timer : expired){
        cbs.push_back(timer->m_cb);
        if(timer->m_recurring){
            timer->m_next = bryant::GetCurrentMS() + timer->m_ms;
            m_timers.insert(timer);
        }
    }

    m_rwMutex.unlock();
}


bool 
TimerManager::hasTimer()
{
    m_rwMutex.readLock();
    bool not_empty = !m_timers.empty();
    m_rwMutex.unlock();
    return not_empty;
}


}