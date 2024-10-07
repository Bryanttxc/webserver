#pragma once

#include <error.h>
#include <pthread.h> // mutex relevant
#include <semaphore.h> // semaphore relevant

namespace bryant{

// 自定义的互斥锁
// ps: 
// 1. 也可以使用std::lock_guard<std::mutex> 来代替
// 2. 也可以使用std::unique_lock<std::mutex> 来代替
class Mutex{
public:
    /**
     * @brief Construct a new Mutex object
     */
    Mutex(){
        // NULL 表示默认属性
        if(pthread_mutex_init(&m_mutex, NULL) != 0){
            perror("pthread_mutex_init");
        };
    }

    /**
    * @brief Destroy the Mutex object
    */
    ~Mutex(){
        if(pthread_mutex_destroy(&m_mutex) != 0){
        }
    }

    /**
    * @brief create lock
    * 
    */
    void lock(){
        if(pthread_mutex_lock(&m_mutex) != 0){
            perror("pthread_mutex_lock");
        }
    }

    /**
    * @brief release lock
    * 
    */
    void unlock(){
        if(pthread_mutex_unlock(&m_mutex) != 0){
            perror("pthread_mutex_unlock");
        }
    }

private:
    pthread_mutex_t m_mutex;
};


// 自定义的信号量
class Semaphore{
public:
    /**
     * @brief Construct a new Semaphore object
     */
     Semaphore(){
        // sem_t *sem
        // pshared=0: 进程中的所有线程共享信号量
        // value=0: 信号量初始值为0
        if(sem_init(&m_sem, 0, 0) != 0){
            perror("sem_init");
        }
    }

    /**
     * @brief Construct a new Semaphore object
     */
     Semaphore(int val){
        // sem_t *sem
        // pshared=0: 进程中的所有线程共享信号量
        // value=0: 信号量初始值为0
        if(sem_init(&m_sem, 0, val) != 0){
            perror("sem_init");
        }
    }

    /**
     * @brief Destroy the Semaphore object
     */
    ~Semaphore(){   
        if(sem_destroy(&m_sem) != 0){
        }
    }

    /**
     * @brief lock a semphore
     * 
     * @return true
     * @return false 
     */
    bool wait(){
        // 等待信号量，属于V操作，阻塞
        return sem_wait(&m_sem) == 0;
    }

    /**
     * @brief lock a semphore with timeout
     * 
     * @return int 0-success，-1-EINVAL，-2-ETIMEDOUT
     */
    int timewait(){
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 10; // 10s
        return sem_timedwait(&m_sem, &ts);
    }

    /**
     * @brief release a semaphore
     * 
     * @return true release success
     * @return false
     */
    bool post(){
        // 释放信号量，属于P操作
        return sem_post(&m_sem) == 0;
    }

private:
    sem_t m_sem;
};


class RWMutex{
public:
    /**
     * @brief Construct a new RWMutex object
     */
     RWMutex(){
        if(pthread_rwlock_init(&m_rwlock, NULL) != 0){
            perror("pthread_rwlock_init");
        }
    }

    /**
     * @brief Destroy the RWMutex object
     */
    ~RWMutex(){
        if(pthread_rwlock_destroy(&m_rwlock) != 0){
            perror("pthread_rwlock_destroy");
        }
    }

    /**
     * @brief read lock
     * 
     */
    void readLock(){
        if(pthread_rwlock_rdlock(&m_rwlock) != 0){
            perror("pthread_rwlock_rdlock");
        }
    }

    /**
     * @brief write lock
     * 
     */
     void writeLock(){
        if(pthread_rwlock_wrlock(&m_rwlock) != 0){
            perror("pthread_rwlock_wrlock");
        }
    }

    /**
     * @brief unlock
     * 
     */
    void unlock(){
        if(pthread_rwlock_unlock(&m_rwlock) != 0){
            perror("pthread_rwlock_unlock");
        }
    }

private:
    pthread_rwlock_t m_rwlock;
};


} // namespace bryant