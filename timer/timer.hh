#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <set>

#include "../util/locker.hh"
#include "../util/util.hh"

namespace bryant{

class TimerManager;

class Timer : public std::enable_shared_from_this<Timer>{

    friend class TimerManager;

public:
    using ptr = std::shared_ptr<Timer>;

    /**
     * @brief cancel this timer
     * 
     */
    bool cancel();

    /**
     * @brief refresh m_next of this timer
     * 
     */
    bool refresh();

    /**
     * @brief reset this timer
     * 
     * @param ms 提供的时间
     * @param from_now 是否要改时间
     * @return true 
     * @return false 
     */
    bool reset(uint64_t ms, bool from_now);

private:
    /**
     * @brief Construct a new Timer
     * 
     * @param ms 
     * @param cb 给定时器绑定回调函数
     * @param recurring 是否循环定时器
     */
    Timer(int ms, std::function<void()> cb, TimerManager* timer_manager, bool recurring = false);

    /**
     * @brief Construct a new Timer
     * 
     * @param next 
     * @return Timer::ptr 
     */
    Timer(uint64_t next);

private:
    /**
    * @brief use to order events with timer
    * 
    */
    struct Comparator{
        bool operator()(const Timer::ptr lhs, const Timer::ptr rhs) const{
            if(!lhs && !rhs){
                return false;
            }
            if(!lhs){
                return true;
            }
            if(!rhs){
                return false;
            }

            if(lhs->m_next < rhs->m_next){
                return true;
            } else if(lhs->m_next > rhs->m_next) {
                return false;
            } else {
                return lhs.get() < rhs.get();
            }
        }
    };

private:
    uint64_t m_ms = 0; // 执行周期
    uint64_t m_next = 0; // 精确的执行时刻
    std::function<void()> m_cb; // 回调函数
    TimerManager* m_manager; // 该定时器的管理者
    bool m_recurring; // 循环定时器
};


class TimerManager{
    
    friend class Timer;

public:
    /**
     * @brief Construct a new Timer Manager
     * 
     */
    TimerManager();

    /**
     * @brief Destroy the Timer Manager
     * 
     */
    virtual ~TimerManager();

    /**
     * @brief add timer
     * 
     * @param ms 
     * @param cb 
     * @param recurring 
     */
    Timer::ptr addTimer(uint64_t ms, std::function<void()> cb, bool recurring = false);

    /**
     * @brief 
     * 
     * @param timer 
     */
    void addTimer(Timer::ptr timer, RWMutex& rw_mutex);

    /**
     * @brief add timer with condition
     * 
     * @param ms 
     * @param cb 
     * @param recurring 
     * @param condition 
     */
    Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> condition, 
                                bool recurring = false);

    /**
     * @brief Get the Next Timer
     * 
     * @return uint64_t 
     */
    uint64_t getNextTimer();

    /**
     * @brief Construct a new list Expired Cb
     * 
     */
    void listExpiredCb(std::vector<std::function<void()>>& cbs);

    /**
     * @brief check if has timer
     * 
     * @return true 
     * @return false 
     */
    bool hasTimer();

protected:
    virtual void onTimerInsertedAtFront() = 0;

private:
    RWMutex m_rwMutex; // 读写锁
    std::set<Timer::ptr, Timer::Comparator> m_timers; // 定时器集合（小根堆）
    uint64_t m_previousTimer; // 记录构造TimerManager对象的时间
    bool m_tickled; // 记录是否触发onTimerInsertedAtFront
};


}