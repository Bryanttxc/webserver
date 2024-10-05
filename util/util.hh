#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace bryant{

/**
 * @brief Get the Thread Id
 * 
 * @return pid_t 
 */
pid_t GetThreadId();


}