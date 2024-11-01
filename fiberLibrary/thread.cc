#include "thread.hh"
#include "../log/logger.hh"
#include "../util/util.hh"

namespace bryant{

static thread_local Thread* t_thread = nullptr; // 当前线程
static thread_local std::string t_thread_name = "UNKNOW"; // 当前线程名


Thread*
Thread::GetThis(){
    return t_thread;
}


const std::string& 
Thread::GetName(){
    // 不用t_thread->m_name是因为主线程不属于Thread类,
    // 没有m_name
    return t_thread_name;
}


void 
Thread::SetName(const std::string& name){
    if(t_thread){
        // 自创建的线程
        t_thread->m_name = name;
    }
    t_thread_name = name; // 主线程/自创建线程
}


pid_t 
Thread::GetThreadId(){
    return bryant::GetThreadId();
}


Thread::Thread(std::function<void()> cb, const std::string& name)
    : m_cb(std::move(cb)){
    if(name.empty()){
        m_name = "UNKNOW";
    } else {
        m_name = name;
    }
    
    int rt = pthread_create(&m_thread, nullptr, run, this);
    if(rt){
        LOG_ERROR("[Thread] pthread_create error");
        throw std::logic_error("pthread_create error");
    }
    
    // 等待线程函数完成初始化，主要是等待cb函数以前的部分
    m_sem.wait();
}


Thread::~Thread(){
    if(m_thread){
        pthread_detach(m_thread);
    }
}


void
Thread::join(){
    if(m_thread){
        int rt = pthread_join(m_thread, nullptr);
        if(rt){
            LOG_ERROR("[Thread] pthread_join error");
            throw std::logic_error("pthread_join error");
        }
    }
    m_thread = 0;
}


void*
Thread::run(void* arg){
    Thread* thread = (Thread*)arg;
    
    t_thread = thread; // 切换为当前线程
    t_thread->m_id = GetThreadId(); // 设置当前线程id
    SetName(thread->m_name); // 设置当前线程名

    // 为了避免直接使用智能指针，而是通过std::function来管理回调函数
    // sylar: 如果m_cb里有智能指针，这样swap可以减少智能指针的引用计数
    std::function<void()> cb;
    cb.swap(thread->m_cb);

    // 初始化完成
    thread->m_sem.post();

    cb();
    return nullptr;
}


}