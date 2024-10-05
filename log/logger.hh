#pragma once

#include <error.h>
#include <unistd.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>

#include "log/block_queue.hh"
#include "util/locker.hh"

namespace bryant{

class Logger{
public:

    /**
     * @brief log level
     * 
     */
    enum LEVEL{
        DEBUG = 0,
        INFO,
        WARNING,
        ERROR,
    };

    /**
     * @brief Get the Instance object
     * 
     * @return Logger* 
     */
    static Logger* getInstance();

    /**
     * @brief initalize relevant parameters
     * 
     */
    bool init(const char* log_name, int max_rows = 50000, int log_buf_size = 8192, int queue_size = 0);

    /**
     * @brief write log, sync/async
     * 
     */
    void write_log(LEVEL level, const char* format, ...);

    /**
     * @brief flush log -> thread callback function
     * 
     */
    static void* flush_log_thread(void * arg);

    /**
     * @brief flush log
     * 
     */
    void flush();

    /**
     * @brief Get the Fp object
     * 
     * @return FILE* 
     */
    FILE* getFp() const {return m_fp;}

private:
    Logger();
    virtual ~Logger();

    /**
     * @brief write log at async mode
     * 
     */
    void async_write_log();

private: // ps: 不太清楚有什么变量
    char m_dirname[128];     //路径名
    char m_logname[128];     //日志文件名
    char m_logFullName[256]; //日志文件路径全名
    int m_count;             //记录当前所在行
    int m_max_rows;          //日志行数记录
    FILE *m_fp;              //打开log的文件指针

    int m_logBuf_size; // 日志缓冲区大小
    char* m_write_buf; // 写缓冲区

    BlockQueue<std::string>* m_block_queue; // 阻塞日志队列
    int max_queue_size; // 队列最大长度
    
    Mutex m_mutex; // 互斥锁
    Semaphore m_sem; // 信号量

    int m_today;        //因为按天分类,记录当前时间是那一天
    int m_close_log;    //关闭日志
    bool m_async_log;   //同步/异步记录日志
};

// 宏定义函数，方便调用
#define LOG_DEBUG(format, ...) \
    do { \
        if(bryant::Logger::getInstance()->getFp() == NULL) { \
            bryant::Logger::getInstance()->init("ServerLog",50000,8192,1000); \
        } \
        bryant::Logger::getInstance()->write_log(bryant::Logger::LEVEL::DEBUG, format, ##__VA_ARGS__); \
    } while(0)


#define LOG_INFO(format, ...) \
    do { \
        if(bryant::Logger::getInstance()->getFp() == NULL) { \
            bryant::Logger::getInstance()->init("ServerLog",50000,8192,1000); \
        } \
        bryant::Logger::getInstance()->write_log(bryant::Logger::LEVEL::INFO, format, ##__VA_ARGS__); \
    } while(0)


#define LOG_WARN(format, ...) \
    do { \
        if(bryant::Logger::getInstance()->getFp() == NULL) { \
            bryant::Logger::getInstance()->init("ServerLog",50000,8192,1000); \
        } \
        bryant::Logger::getInstance()->write_log(bryant::Logger::LEVEL::WARNING, format, ##__VA_ARGS__); \
    } while(0)


#define LOG_ERROR(format, ...) \
    do { \
        if(bryant::Logger::getInstance()->getFp() == NULL) { \
            bryant::Logger::getInstance()->init("ServerLog",50000,8192,1000); \
        } \
        bryant::Logger::getInstance()->write_log(bryant::Logger::LEVEL::ERROR, format, ##__VA_ARGS__); \
    } while(0)

}  // namespace bryant