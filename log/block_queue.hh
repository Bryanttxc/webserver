#pragma once

#include <vector>

#include "util/locker.hh"

namespace bryant{

template <typename T>
class BlockQueue{
public:
    BlockQueue(int max_size = 1000);
    ~BlockQueue();

    /**
     * @brief check if queue is full
     * 
     * @return true 
     * @return false 
     */
    bool isFull();

    /**
     * @brief check if queue is empty
     * 
     * @return true 
     * @return false 
     */
    bool isEmpty();

    /**
     * @brief enqueue
     * 
     * @param data 
     */
    bool enqueue(T& data);
    
    /**
     * @brief dequeue
     * 
     * @return T 
     */
    bool dequeue(T& data);

    /**
     * @brief get the first element
     * 
     * @return T 
     */
    T front();

    /**
     * @brief get the maximum size
     * 
     * @return int 
     */
    int max_size() const {return m_maxSize;}

private:
    std::vector<T> m_data; // 队列数据
    int head; // 队列头
    int tail; // 队列尾
    int m_maxSize; // 队列最大容量
    Mutex m_mutex;
};


template <typename T>
BlockQueue<T>::BlockQueue(int max_size): m_maxSize(max_size){
    if(m_maxSize <= 0){
        perror("max_size must greater than 0");
    }
    m_data.resize(m_maxSize);
    head = 0;
    tail = 0;
}


template <typename T>
BlockQueue<T>::~BlockQueue(){
    m_data.clear();
}


template <typename T>
bool 
BlockQueue<T>::isFull(){
    // head和tail不指向一起
    // tail再往前挪一步就与head重合
    return (tail + 1) % m_maxSize == head;
}


template <typename T>
bool 
BlockQueue<T>::isEmpty(){
    return head == tail;
}


template <typename T>
bool
BlockQueue<T>::enqueue(T& data){
    // tail移动
    m_mutex.lock();
    if(isFull()){
        m_mutex.unlock();
        return false;
    }

    m_data[tail] = data;
    tail = (tail + 1) % m_maxSize;
    m_mutex.unlock();

    return true;
}


template <typename T>
bool
BlockQueue<T>::dequeue(T& data){
    // head移动
    m_mutex.lock();
    if(isEmpty()){
        m_mutex.unlock();
        return false;
    }

    data = m_data[head];
    head = (head + 1) % m_maxSize;
    m_mutex.unlock();

    return true;
}


template <typename T>
T 
BlockQueue<T>::front(){
    m_mutex.lock();
    if(isEmpty()){
        m_mutex.unlock();
        return NULL;
    }
    T data = m_data[head];
    m_mutex.unlock();

    return data;
}

}