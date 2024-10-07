#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>

namespace bryant{

/**
 * @brief Get the Thread Id
 * 
 * @return pid_t 
 */
pid_t GetThreadId();

/**
 * @brief Get the Current ms
 * 
 * @return uint64_t 
 */
uint64_t GetCurrentMS();

/**
 * @brief Get the Current us
 * 
 * @return uint64_t 
 */
uint64_t GetCurrentUS();

}