#include "iomanager.hh"


void test_fiber(){
    LOG_DEBUG("test_fiber");
}


void test_iomanager(){
    bryant::IOManager iom;
    iom.schedule(&test_fiber);
}


int main(){
    test_iomanager();
    return 0;
}