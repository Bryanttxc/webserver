#include "util.hh"

namespace bryant{


pid_t GetThreadId(){
    return syscall(SYS_gettid);
}



}