
#include "log/logger.hh"
#include "log/block_queue.hh"

void test_logger(){
    LOG_DEBUG("%s","test Debug!");
    LOG_INFO("%s","test Info!");
    LOG_WARN("%s","test Warn!");
    LOG_ERROR("%s","test Error!");
}

void test_blockQueue(){
    // case1: enqueue 5 elements, dequeue 6 elements
    bryant::BlockQueue<int> bq(5);
    printf("case1: enqueue 5 elements, dequeue 6 elements\n");
    for(int i = 0; i < 5; i++){
        bq.enqueue(i);
    }
    printf("dequeue: ");
    int tmp = 0;
    for(int i = 0; i < 6; i++){
        if(bq.dequeue(tmp)){
            printf("%d ",tmp);
        } else {
            printf("null ");
        }
    }
    printf("\n");

    // case2: enqueue 6 elements but size = 5
    printf("case2: enqueue 6 elements but size = 5\n");
    for(int i = 0; i < 6; i++){
        bq.enqueue(i);
    }
    tmp = 0;
    printf("dequeue: ");
    for(int i = 0; i < 6; i++){
        if(bq.dequeue(tmp)){
            printf("%d ",tmp);
        } else {
            printf("null ");
        }
    }
    printf("\n");
}

int main(){
    test_logger();
    test_blockQueue();
}