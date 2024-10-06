#include <unistd.h>
#include <vector>
#include <string>

#include "../fiberLibrary/scheduler.hh"
#include "../log/logger.hh"


void test_fiber() {
    static int s_count = 5;
    LOG_DEBUG("[Fiber] test in fiber %lu s_count=%d\n", bryant::Fiber::GetThis()->getId(), s_count);
    // sleep(1);
    if(--s_count >= 0) {
        bryant::Scheduler::GetThis()->schedule(&test_fiber);
    }
}


void test_scheduler(){
    bryant::Scheduler scheduler(1, true, "test");
    LOG_DEBUG("Main Thread '%lu', name: '%s' created scheduler", 
                bryant::GetThreadId(), scheduler.getName().c_str());
    scheduler.start();
    LOG_DEBUG("Main Thread scheduler '%s' finish to start", scheduler.getName().c_str());
    scheduler.schedule(&test_fiber);
    LOG_DEBUG("Main Thread scheduler '%s' finish to schedule", scheduler.getName().c_str());
    sleep(1);
    scheduler.stop();
    LOG_DEBUG("Main Thread scheduler '%s' finish to stop", scheduler.getName().c_str());
}


int main(){
    test_scheduler();
    return 0;
}