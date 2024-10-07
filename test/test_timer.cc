#include "fiberLibrary/iomanager.hh"
#include "timer/timer.hh"

bryant::Timer::ptr s_timer;

void test_timer(){
    bryant::IOManager iom(2);
    s_timer = iom.addTimer(1000, [](){
        static int i = 0;
        printf("[Test] hello timer i=%d\n", i);
        if(++i == 3) {
            s_timer->reset(2000, true);
            //s_timer->cancel();
        }
    }, true);
}


int main(){
    test_timer();
    return 0;
}