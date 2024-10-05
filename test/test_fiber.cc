#include <string>

#include "../log/logger.hh"
#include "../fiberLibrary/fiber.hh"
#include "../fiberLibrary/thread.hh"


// cb
void run_in_fiber(){
    LOG_INFO("Fiber %d run_in_fiber begin", bryant::Fiber::GetThis()->getId());
    bryant::Fiber::GetThis()->yield(); // 任务协程第一次让出执行权
    LOG_INFO("Fiber %d run_in_fiber end", bryant::Fiber::GetThis()->getId());
}


// fiber
void test_fiber(){
    bryant::Fiber::GetThis(); // 创建了线程主协程
    LOG_INFO("Thread %s main Fiber %lu begin", 
        bryant::Thread::GetThis()->getName().c_str(),
        bryant::Fiber::GetThis()->getId());
    
    bryant::Fiber::ptr fiber(new bryant::Fiber(run_in_fiber)); // 任务协程

    fiber->resume(); // 任务协程第一次执行
    LOG_INFO("Thread %s main Fiber %lu after first resume", 
        bryant::Thread::GetThis()->getName().c_str(),
        bryant::Fiber::GetThis()->getId());

    fiber->resume(); // 任务协程第二次执行
    LOG_INFO("Thread %s main Fiber %lu after second resume", 
        bryant::Thread::GetThis()->getName().c_str(),
        bryant::Fiber::GetThis()->getId());
}


int main(){
    bryant::Thread::SetName("main");
    LOG_INFO("Now in Thread %s", bryant::Thread::GetName().c_str());

    std::vector<bryant::Thread::ptr> thrs;
    for(int i = 0; i < 3; ++i){
        thrs.push_back(bryant::Thread::ptr(
            new bryant::Thread(&test_fiber, "name_" + std::to_string(i))
        ));
    }

    for(int i = 0; i < 3; ++i){
        thrs[i]->join();
    }

    return 0;
}