#include "logger.hh"

namespace bryant{

Logger::Logger(){
    m_count = 0;
    m_async_log = false;
}


Logger::~Logger(){
    if(m_fp != NULL){
        fclose(m_fp);
    }
}


Logger*
Logger::getInstance(){
    // C++11局部懒汉不用加锁
    static Logger logger;
    return &logger;
}


bool
Logger::init(const char* log_name, int max_rows, int log_buf_size)
{
    // 变量初始化
    m_max_rows = max_rows;
    m_logBuf_size = log_buf_size;
    m_write_buf = new char[m_logBuf_size]; // cxt: 一开始没加
    memset(m_write_buf, '\0', m_logBuf_size); // 全部赋值为'/0'

    // 日志名初始化
    /// 获取当前时间
    time_t now = time(NULL); 
    struct tm* local_time;
    local_time = localtime(&now); // 转换为本地时间
    m_today = local_time->tm_mday; // 更新m_today

    char buf[16];
    strftime(buf, sizeof(buf), "%Y%m%d", local_time); // 转换成年-月-日的时间格式
    
    /// 初始化日志名称
    const char *path_end = strrchr(log_name, '/'); // 获取指定路径

    if(path_end == NULL){
        snprintf(m_logFullName, sizeof(m_logFullName), "%s%s_%s", "./info/", buf, log_name);
    } else {
        strncpy(m_dirname, log_name, path_end - log_name + 1); // 截取
        strcpy(m_logname, path_end + 1); // 拷贝
        snprintf(m_logFullName, sizeof(m_logFullName), "%s%s%s_%s", "./info/", m_dirname, buf, m_logname);
    }

    /// 打开指定文件，以追加写的方式
    m_fp = fopen(m_logFullName, "a");
    if(m_fp == NULL){
        perror("fopen");
        return false;
    }
    
    return true;
}


// 已解决：
// 1. 在哪加锁：写日志的时候
// 2. 一句日志需要：时刻，日志级别，日志内容
void
Logger::write_log(LEVEL level, const char* format, ...)
{
    // 获取当前时间
    time_t now = time(NULL);
    struct tm* local_time;
    local_time = localtime(&now);
    char buf[32] = {0};
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", local_time); // 转换成“年-月-日 时:分:秒”的时间格式

    // 设置日志级别
    char level_info[16] = {0};
    switch(level){
        case LEVEL::DEBUG:
            strcpy(level_info, "[DEBUG]: ");
            break;
        case LEVEL::INFO:
            strcpy(level_info, "[INFO]: ");
            break;
        case LEVEL::WARNING:
            strcpy(level_info, "[WARNING]: ");
            break;
        case LEVEL::ERROR:
            strcpy(level_info, "[ERROR]: ");
            break;
        default:
            break;
    }

    // 开始写日志
    m_mutex.lock();

    m_count++;

    /// 如遇到下列两种情况需要重新创建新的文件
    if(local_time->tm_mday != m_today || m_count % m_max_rows == 0){
        fflush(m_fp);
        fclose(m_fp);
        
        char date[16] = {0};
        snprintf(date, sizeof(date), "%d%02d%02d", local_time->tm_year + 1900, local_time->tm_mon + 1, local_time->tm_mday);

        if(local_time->tm_mday != m_today){
            // 当前时间不等于m_today
            m_today = local_time->tm_mday;    
            snprintf(m_logFullName, sizeof(m_logFullName), "%s%s%s_%s", "./info/", m_dirname, date, m_logname);
        } else {
            // 当前行数已经到头了
            snprintf(m_logFullName, sizeof(m_logFullName), "%s%s%s_%s_%d", "./info/", m_dirname, date, m_logname, m_count / m_max_rows);
        }

        m_count = 0;

        m_fp = fopen(m_logFullName, "a");
        if(m_fp == NULL){
            perror("fopen");
        }
    }

    /// 接收变长的传参
    va_list args;
    va_start(args, format);

    int n = snprintf(m_write_buf, m_logBuf_size - 1, "%s%s", buf, level_info);
    int m = vsnprintf(m_write_buf + n, m_logBuf_size - 1, format, args);
    m_write_buf[n + m] = '\n';
    m_write_buf[n + m + 1] = '\0';

    va_end(args);

    if(m_async_log){
        // 异步
        char content[1024] = {0};
        snprintf(content, sizeof(content), "%s", m_write_buf);
        std::string str = content;
        m_block_queue->enqueue(str);
    } else {
        // 同步
        fputs(m_write_buf, m_fp);
    }

    m_mutex.unlock();
}


// 已解决：这个是thread的callback
void* 
Logger::flush_log_thread(void* arg){
    Logger::getInstance()->async_write_log();
    return nullptr;
}


void
Logger::flush(){
    m_mutex.lock();
    fflush(m_fp);
    m_mutex.unlock();
}


void 
Logger::async_write_log(){
    std::string content; // 这样可以减少拷贝次数
    while(m_block_queue->dequeue(content)){
        m_mutex.lock();
        fputs(content.c_str(), m_fp);
        m_mutex.unlock();
    }
}

}