#pragma once

#include <unistd.h>     //有fork（）这些函数
#include <signal.h>
#include <sys/types.h>  //其中最常见的是pid_t、size_t、ssize_t等类型的定义，这些类型在系统编程中经常用于表示进程ID、大小和有符号大小等。
#include <sys/epoll.h>
#include <fcntl.h>      //用于对已打开的文件描述符进行各种控制操作，如设置文件描述符的状态标志、非阻塞I/O等。
#include <sys/socket.h>
#include <netinet/in.h> //Internet地址族的相关结构体和符号常量,其中最常见的结构体是sockaddr_in，用于表示IPv4地址和端口号。用于网络编程中
#include <arpa/inet.h>  //声明了一些与IPv4地址转换相关的函数
#include <assert.h>     //该头文件声明了一个宏assert()，用于在程序中插入断言
#include <sys/stat.h>   //用于获取文件的状态信息，如文件的大小、访问权限等。
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>   //该头文件声明了一些与进程等待相关的函数和宏，其中最常用的函数是waitpid()，用于等待指定的子进程状态发生变化
#include <sys/uio.h>
// 该头文件声明了一些与I/O向量操作相关的函数和结构体。
// 其中最常用的结构体是iovec，它用于在一次系统调用中传输多个非连续内存区域的数据
// 例如在网络编程中使用readv()和writev()函数来进行分散读取和聚集写入。
#include <map>
#include <mysql/mysql.h>

#include "../util/socket.hh"
#include "../util/locker.hh"
#include "../fiberLibrary/iomanager.hh"
#include "../log/logger.hh"
#include "../CGImysql/sql_connection_pool.hh"


namespace bryant{

class http_conn {
public:
    static const int FILENAME_LEN = 200;       // 文件名长度 静态即初始化
    static const int READ_BUFFER_SIZE = 2048;  // 读入的缓冲区大小
    static const int WRITE_BUFFER_SIZE = 1024; // 写入的缓冲区大小
    
    // http请求方法
    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };

    // 有限状态机：行，头，体
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };

    // 状态码
    enum HTTP_CODE
    {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };

    // 行状态
    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

public:
    http_conn() = default;
    ~http_conn() = default;

public:
    /**
     * @brief initialize connect instance
     * 
     * @param client socket object
     * @param user 
     * @param passwd 
     * @param sqlname 
     * @param keepalive whether if keep long connection 
     */
    void init(sylar::Socket::ptr &client, char* root, int TRIGMode, 
            std::string user, std::string passwd, std::string sqlname, 
            bool keepalive);
    
    /**
     * @brief close connection instance
     * 
     * @param real_close 
     */
    void close_conn(bool real_close = true);

    /**
     * @brief process
     * 
     * @return true 
     * @return false 
     */
    bool process();

    /**
     * @brief read function in ET mode
     * 
     * @return true 
     * @return false 
     */
    bool read_once();

    /**
     * @brief write function
     * 
     * @return true 
     * @return false 
     */
    bool write();

    /**
     * @brief Get the address
     * 
     * @return sockaddr_in* 
     */
    sockaddr_in *get_address()
    {
        return &m_address;
    }

    /**
     * @brief initialize mysql result
     * 
     * @param conn_pool 
     */
    void initmysql_result(Connection_pool* conn_pool);

    /**
     * @brief Get the m_read_buf
     * 
     * @return const char* 
     */
    const char* getReadBuf() const {return m_read_buf;}

public:
    int timer_flag; // 用于定时器
    int improv;

    static int m_user_count;    // conn连接数
    MYSQL* mysql;               // mysql对象
    int m_state;                // 读为0, 写为1, 设置状态的

private:
    void init();

    // 解析请求
    HTTP_CODE process_read();                                // 进程读取

    HTTP_CODE do_request(bool &decide_proxy);                // 解析请求(总)->有限状态机：行、头、体
    HTTP_CODE parse_request_line(char* text);                // 解析请求行
    HTTP_CODE parse_headers(char* text,bool& decide_proxy);  // 后续多加了代理类，可以将bool改成int // 解析头部
    HTTP_CODE parse_content(char* text);                     // 解析请求体

    // tool
    char* get_line() { return m_read_buf + m_start_line; };  // 获取行
    LINE_STATUS parse_line();                                // 解析行
    void unmap();

    // 响应请求
    bool process_write(HTTP_CODE ret);                       // 进程写入

    bool add_response(const char* format, ...);              // 生成响应(总)->有限状态机：行、头、体
    bool add_status_line(int status, const char* title);
    bool add_headers(int content_length);
    bool add_content(const char* content);
    
    // tool
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger(); // 是否keep-Alive
    bool add_blank_line();

private:
    sylar::Socket::ptr m_client;            // 套接字的文件描述符对应的封装
    sockaddr_in m_address;                  // 为什么：用户socket地址？

    char m_read_buf[READ_BUFFER_SIZE];      // 读缓冲区
    long m_read_idx;                        // 记录当前读取的下标位置
    long m_checked_idx;                     // 记录当前位置
    int m_start_line;                       // 记录开始行

    char *m_url;                            // 存储URL
    char *m_version;                        // 存储HTTP版本
    char *m_host;                           // 存储host名
    long m_content_length;                  // 存储报文内容长度
    bool m_linger;                          // 是否keep-Alive
    char* m_string;                         // 存储请求头数据

    char *m_file_address;                   // 为什么：文件描述符的地址？
    struct stat m_file_stat;                // 文件状态
    char m_real_file[FILENAME_LEN];         // 文件
    int cgi;                                // 是否启用的post
    char* doc_root;                         // 已解决:服务器根目录

    char m_write_buf[WRITE_BUFFER_SIZE];    // 写缓冲区
    int m_write_idx;                        // 记录当前写入的下标位置
    CHECK_STATE m_check_state;              // 当前状态：行，头，体
    METHOD m_method;                        // 返回一个请求方法类型

    struct iovec m_iv[2];                   // writev相关
    int m_iv_count;                         // writev相关
    int bytes_to_send;                      // 当前待发送的字节数
    int bytes_have_send;                    // 当前已发送的字节数

    int m_TRIGMode;                         // 触发模式

    char sql_user[100];                     // SQL用户
    char sql_passwd[100];                   // SQL密码
    char sql_name[100];                     // SQL名
};

} // namespace bryant