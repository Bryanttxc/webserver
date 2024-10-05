#include "../fiberLibrary/thread.hh"
#include "../log/logger.hh"

void fun1(){
    LOG_INFO("thread %d is running\n \
              this.id: %d\n \
              thread name: %s\n \
              this.name: %s",
                bryant::Thread::GetThreadId(),
                bryant::Thread::GetThis()->getId(),
                bryant::Thread::GetName().c_str(),
                bryant::Thread::GetThis()->getName().c_str()
            );
}


int main(){
    LOG_INFO("test_thread begin");

    std::vector<bryant::Thread::ptr> thread_arr;

    for(int i = 0; i < 5; ++i){
        bryant::Thread::ptr thread(new bryant::Thread(&fun1, "name_" + std::to_string(i)));
        thread_arr.push_back(thread);
    }

    for(int i = 0; i < 5; ++i){
        thread_arr[i]->join();
    }

    LOG_INFO("test_thread end");
    return 0;
}