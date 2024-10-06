#include <error.h>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../util/locker.hh"


/// sell ticket example ///
int ticket = 500; // 总门票数
bryant::Mutex mutex; // 互斥锁


/**
 * @brief callback function of thread
 * 
 * @param arg not need
 * @return void* 
 */
void* work(void* arg){
    while(true){
        mutex.lock();
        if(ticket > 0){
            printf("thread: %lu sell ticket %d\n", (unsigned long)pthread_self(), ticket);
            --ticket;
        } else{
            mutex.unlock();
            break;
        }
        mutex.unlock();
        usleep(3000);
    }
    return nullptr;
}


/**
 * @brief test function of class Mutex
 * 
 * @param thread_count 
 */
void test_mutex(int thread_count){
    if(thread_count <= 0){
        printf("thread count must be positive.\n");
        return;
    }

    std::unique_ptr<pthread_t[]> m_threads(new pthread_t[thread_count]);
    for(int i = 0; i < thread_count; ++i){
        if(pthread_create(m_threads.get() + i, NULL, work, NULL)){
            perror("pthread create");
            return;
        }
    }

    for(int i = 0; i < thread_count; ++i){
        if(pthread_join(m_threads[i], NULL)){
            perror("pthread join");
            return;
        }
    }

    // printf("All threads are finished. Exiting main thread.\n");
}


/// producter - consumer example ///
/**
* @brief 生产者-消费者模型的公有队列
*/
struct Node{
    int val;
    Node* next;
    Node():val(0), next(NULL){}
    Node(int _val):val(_val), next(NULL){}
};


/**
 * @brief 包含node和信号量的队列
 * 
 */
struct SemQueue{
    Node* m_node;
    bryant::Semaphore psem{2};
    bryant::Semaphore csem{0};
    SemQueue(){}
    SemQueue(Node* _node):m_node(_node){}
    ~SemQueue(){
        delete m_node;
        m_node = nullptr;
    }
};


/**
 * @brief callback function of producer
 * 
 * @param arg 
 * @return void* 
 */
void* sem_producer(void* arg){
    SemQueue* sem_queue = (SemQueue*)arg;
    int ret = 0;
    int cnt = 0; // 用于计数

    while(cnt <= 10000){
        ret = sem_queue->psem.timewait(); // 等待消费者的信号量
        if(ret != 0){
            if(ret == ETIMEDOUT){
                printf("consumer close\n");
                break;
            }
            perror("psem wait");
        }
        
        mutex.lock();
        Node* newNode = new Node();
        newNode->next = sem_queue->m_node;
        sem_queue->m_node = newNode;
        newNode->val = rand() % 1000;
        printf("add node val is %d by thread %lu\n", newNode->val, (unsigned long)pthread_self());
        mutex.unlock();

        ret = sem_queue->csem.post();
        if(!ret){
            perror("psem wait");
        }

        ++cnt;
    }

    // printf("producer: %d\n", cnt);
    return nullptr;
}


/**
 * @brief callback function of consumer
 * 
 * @param arg 
 * @return void* 
 */
void* sem_consumer(void* arg){
    SemQueue* sem_queue = (SemQueue*)arg;
    int ret = 0;
    int cnt = 0;

    while(cnt <= 10000){
        ret = sem_queue->csem.timewait();
        if(ret != 0){
            if(ret == ETIMEDOUT){
                printf("producer close\n");
                break;
            }
            perror("csem wait");
        }

        mutex.lock();
        Node* tmp = sem_queue->m_node;
        sem_queue->m_node = sem_queue->m_node->next;
        printf("get node val is %d by thread %ld\n", tmp->val, pthread_self());
        delete tmp;
        mutex.unlock();

        ret = sem_queue->psem.post();
        if(!ret){
            perror("psem wait");
        }

        ++cnt;
    }

    // printf("consumer: %d\n", cnt);
    return nullptr;
}


void test_semaphore(int thread_count){
    std::unique_ptr<SemQueue> sem_queue = std::make_unique<SemQueue>();
    std::unique_ptr<pthread_t[]> m_threads(new pthread_t[thread_count]);

    if(pthread_create(&m_threads[0], NULL, sem_producer, sem_queue.get()) != 0){
        perror("pthread_create");
        return;
    }

    if(pthread_create(&m_threads[1], NULL, sem_consumer, sem_queue.get()) != 0){
        perror("pthread_create");
        return;
    }

    for(int i = 0; i < thread_count; i++){
        if(pthread_join(m_threads[i], NULL)){
            perror("pthread_join");
            break;
        }
    }
}


int main(){
    printf("start to test Mutex...\n");
    test_mutex(3);

    usleep(3000);

    printf("start to test Semaphore...\n");
    test_semaphore(2);

    printf("success!\n");
    
    return 0;
}