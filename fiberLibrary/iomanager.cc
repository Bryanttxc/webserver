#include <cassert>

#include "iomanager.hh"

namespace bryant{


IOManager::IOManager(int num_thread, bool use_caller, const std::string& name) 
        :Scheduler(num_thread, use_caller, name) {
    
    // 创建epoll
    m_epollfd = epoll_create(8);
    if (m_epollfd < 0) {
        throw std::logic_error("epoll_create");
    }

    // 创建管道
    int rt = pipe(m_tickleFds);
    if (rt < 0) {
        throw std::logic_error("pipe");
    }

    /// 创建关于管道的读事件
    epoll_event ep_event;
    ep_event.events = EPOLLIN | EPOLLET;
    ep_event.data.fd = m_tickleFds[0];

    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    if (rt < 0) {
        throw std::logic_error("fcntl");
    }
    rt = epoll_ctl(m_epollfd, EPOLL_CTL_ADD, m_tickleFds[0], &ep_event);
    if (rt < 0) {
        throw std::logic_error("epoll_ctl");
    }

    // 给fd上下文数组设置大小
    contextResize(32);

    // 开启调度器
    start();
}


IOManager::~IOManager() {
    stop();
    close(m_epollfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    for (auto& fd_ctx : m_fdContexts) {
        delete fd_ctx;
    }
}


void
IOManager::contextResize(size_t size){
    m_fdContexts.resize(size);

    for(size_t i = 0; i < m_fdContexts.size(); ++i){
        if(m_fdContexts[i] == nullptr){
            m_fdContexts[i] = new FdContexts();
            m_fdContexts[i]->fd = i;
        }
    }
}


int 
IOManager::addEvent(int fd, Event event, std::function<void()> cb){
    // 根据fd获取指定fd上下文
    FdContexts* fd_ctx = nullptr;
    m_rwMutex.readLock();
    if(m_fdContexts.size() > fd){
        // fd存在
        fd_ctx = m_fdContexts[fd];
        m_rwMutex.unlock();
    } else {
        // fd不存在
        m_rwMutex.unlock();
        m_rwMutex.writeLock();
        contextResize(m_fdContexts.size() * 1.5); // 扩容
        fd_ctx = m_fdContexts[fd];
        m_rwMutex.unlock();
    }

    fd_ctx->mutex.lock();

    // 检测事件重复
    if(fd_ctx->events & event){
        LOG_ERROR("addEvent assert fd=%d, event=%d, fd_ctx.event=%d", 
                    fd, (EPOLL_EVENTS)event, (EPOLL_EVENTS)fd_ctx->events);
        fd_ctx->mutex.unlock();
        assert(!(fd_ctx->events & event));
    }

    // 创建事件
    epoll_event ep_event;
    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    ep_event.events = EPOLLET | fd_ctx->events | event;
    ep_event.data.ptr = fd_ctx;
    int rt = epoll_ctl(m_epollfd, op, fd, &ep_event);
    if(rt){
        LOG_ERROR("epoll_ctl(%d, %d, %d): rt:%d (errno=%d, str=%s) fd_ctx->events=%d",
                    m_epollfd, op, (EPOLL_EVENTS)ep_event.events, rt, errno,
                    strerror(errno), (EPOLL_EVENTS)fd_ctx->events);
        fd_ctx->mutex.unlock();
        return -1;
    }

    // 设置事件
    ++m_pendingEventCount;
    fd_ctx->events = (Event)(fd_ctx->events | event);
    FdContexts::EventContext& event_ctx = fd_ctx->getEventContext(event);
    assert(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);

    event_ctx.scheduler = Scheduler::GetThis();
    if(cb){
        event_ctx.cb.swap(cb);
    } else {
        event_ctx.fiber = Fiber::GetThis();
        assert(event_ctx.fiber->getState() == Fiber::RUNNING);
    }

    fd_ctx->mutex.unlock();
    return 0;
}


bool 
IOManager::delEvent(int fd, Event event){
    // 根据fd获取指定fd上下文
    m_rwMutex.readLock();
    if(m_fdContexts.size() <= fd){
        return false;
    }
    FdContexts* fd_ctx = m_fdContexts[fd];
    m_rwMutex.unlock();

    fd_ctx->mutex.lock();

    // 检测事件是否存在
    if(!(fd_ctx->events & event)){
        LOG_ERROR("delEvent assert fd=%d, event=%d, fd_ctx.event=%d", 
                    fd, (EPOLL_EVENTS)event, (EPOLL_EVENTS)fd_ctx->events);
        assert(fd_ctx->events & event);
    }

    // 删除事件
    epoll_event ep_event;
    Event new_event = (Event)(fd_ctx->events & ~event);
    int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL; 
    ep_event.events = EPOLLET | new_event;
    ep_event.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epollfd, op, fd, &ep_event);
    if(rt){
        LOG_ERROR("epoll_ctl(%d, %d, %d): rt:%d (errno=%d, str=%s) fd_ctx->events=%d",
            m_epollfd, op, (EPOLL_EVENTS)ep_event.events, rt, errno,
            strerror(errno), (EPOLL_EVENTS)fd_ctx->events);
        fd_ctx->mutex.unlock();
        return false;
    }

    // 设置事件
    --m_pendingEventCount;
    fd_ctx->events = new_event;
    FdContexts::EventContext& event_ctx = fd_ctx->getEventContext(event);
    fd_ctx->resetEventContext(event_ctx);

    fd_ctx->mutex.unlock();
    return true;
}


bool 
IOManager::cancelEvent(int fd, Event event){
    // 根据fd获取指定fd上下文
    m_rwMutex.readLock();
    if(m_fdContexts.size() <= fd){
        return false;
    }

    FdContexts* fd_ctx = m_fdContexts[fd];
    m_rwMutex.unlock();

    fd_ctx->mutex.lock();

    // 检测事件是否存在
    if(!(fd_ctx->events & event)){
        LOG_ERROR("cancelEvent assert fd=%d, event=%d, fd_ctx.event=%d", 
                    fd, (EPOLL_EVENTS)event, (EPOLL_EVENTS)fd_ctx->events);
        assert(fd_ctx->events & event);
    }

    // 删除事件
    epoll_event ep_event;
    Event new_event = (Event)(fd_ctx->events & ~event);
    int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    ep_event.events = EPOLLET | new_event;
    ep_event.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epollfd, op, fd, &ep_event);
    if(rt){
        LOG_ERROR("epoll_ctl(%d, %d, %d): rt:%d (errno=%d, str=%s) fd_ctx->events=%d",
            m_epollfd, op, (EPOLL_EVENTS)ep_event.events, rt, errno,
            strerror(errno), (EPOLL_EVENTS)fd_ctx->events);
        fd_ctx->mutex.unlock();
        return false;
    }

    // 最后一次触发事件
    fd_ctx->triggerEvent(event);
    --m_pendingEventCount;
    
    fd_ctx->mutex.unlock();
    return true;
}


bool 
IOManager::cancelAll(int fd){
    // 根据fd获取指定fd上下文
    m_rwMutex.readLock();
    if(m_fdContexts.size() <= fd){
        return false;
    }
    FdContexts* fd_ctx = m_fdContexts[fd];
    m_rwMutex.unlock();

    fd_ctx->mutex.lock();
    if(!fd_ctx->events){
        fd_ctx->mutex.unlock();
        return false;
    }

    // 删除全部事件
    epoll_event ep_event;
    int op = EPOLL_CTL_DEL;
    ep_event.events = 0;
    ep_event.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epollfd, op, fd, &ep_event);
    if(rt){
        LOG_ERROR("epoll_ctl(%d, %d, %d): rt:%d (errno=%d, str=%s) fd_ctx->events=%d",
            m_epollfd, op, (EPOLL_EVENTS)ep_event.events, rt, errno,
            strerror(errno), (EPOLL_EVENTS)fd_ctx->events);
        fd_ctx->mutex.unlock();
        return false;
    }

    // 最后一次触发事件
    if(fd_ctx->events & READ){
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
    }
    if(fd_ctx->events & WRITE){
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }

    assert(fd_ctx->events == 0);
    fd_ctx->mutex.unlock();
    return true;
}


IOManager::FdContexts::EventContext& 
IOManager::FdContexts::getEventContext(IOManager::Event event){
    switch(event){
        case READ:
            return read;
        case WRITE:
            return write;
        default:
            assert(false);
    }
}


void 
IOManager::FdContexts::resetEventContext(IOManager::FdContexts::EventContext& eventContext){
    eventContext.cb = nullptr;
    eventContext.fiber.reset();
    eventContext.scheduler = nullptr;
}


void 
IOManager::FdContexts::triggerEvent(IOManager::Event event){
    assert(events & event);
    events = (Event)(events & ~event);
    EventContext& eventContext = getEventContext(event);
    if(eventContext.cb){
        eventContext.scheduler->schedule(&eventContext.cb);
    } else {
        eventContext.scheduler->schedule(&eventContext.fiber);
    }
    
    eventContext.scheduler = nullptr;
}


IOManager*
IOManager::GetThis(){
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}


void
IOManager::tickle() {
    if(hasIdleThreads()){
        return;
    }
    int rt = write(m_tickleFds[1], "T", 1);
    assert(rt == 1);
}


bool 
IOManager::stopping(){
    return Scheduler::stopping() && m_pendingEventCount == 0;
}


// 其实就是idle协程的mainFun()
void
IOManager::idle() {
    LOG_DEBUG("[Fiber] idle Fiber %lu is running", Fiber::GetThis()->getId());

    // 创建epoll_event结构体
    epoll_event* events = new epoll_event[m_max_events];
    std::shared_ptr<epoll_event> shared_event(events, [](epoll_event* ptr){ 
        delete[] ptr; 
    });
    
    // 主循环
    while(true){
        if(stopping()){
            // 停止调度器
            break;
        }

        static const int max_timeout = 5000;
        int rt = epoll_wait(m_epollfd, events, m_max_events, max_timeout);
        if(rt < 0){
            if(errno == EINTR){
                continue;
            }

            LOG_ERROR("epoll_wait(%d, %p, %d, %d): rt:%d (errno=%d, str=%s)",
                m_epollfd, events, 64, max_timeout, rt, errno, strerror(errno));
            break;
        }

        for(int i = 0; i < rt; ++i){
            epoll_event& event = events[i];

            // 读取管道数据
            if(event.data.fd == m_tickleFds[0]){
                uint8_t dummy;
                while(read(m_tickleFds[0], &dummy, 1) > 0){
                    ;
                }
                continue;
            }

            // 有其他事件
            FdContexts* fd_ctx = (FdContexts*)event.data.ptr;

            fd_ctx->mutex.lock();
            if(event.events & (EPOLLERR | EPOLLHUP)){
                event.events |= EPOLLIN | EPOLLOUT;
            }

            // 将读写事件抽取出来
            int real_event = Event::NONE;
            if(event.events & EPOLLIN){
                real_event |= READ;
            }
            if(event.events & EPOLLOUT){
                real_event |= WRITE;
            }

            // 如果fd_ctx->events没有读写事件就跳过
            if((fd_ctx->events & real_event) == NONE){
                continue;
            }

            // 剩余事件重新添加回epoll
            epoll_event ep_event;
            Event left_event = (Event)(fd_ctx->events & (~real_event));
            int op = left_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            ep_event.events = EPOLLET | left_event;
            ep_event.data.ptr = fd_ctx;
            int rt2 = epoll_ctl(m_epollfd, op, fd_ctx->fd, &ep_event);
            if(rt2){
                LOG_ERROR("epoll_ctl(%d, %d, %d): rt:%d (errno=%d, str=%s) fd_ctx->events=%d",
                    m_epollfd, op, (EPOLL_EVENTS)ep_event.events, rt2, errno,
                    strerror(errno), (EPOLL_EVENTS)fd_ctx->events);
                fd_ctx->mutex.unlock();
                continue;
            }

            // 触发event
            if(real_event & READ){
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }
            if(real_event & WRITE){
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }

            fd_ctx->mutex.unlock();
        } // end for

        // 退出idle协程，让给其他协程执行
        Fiber::ptr fiber = Fiber::GetThis();
        auto raw_ptr = fiber.get();
        fiber.reset();
        raw_ptr->yield();

    } // end while(true)
}


};