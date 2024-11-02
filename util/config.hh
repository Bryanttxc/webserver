#pragma once

#include <unistd.h>

namespace bryant{

class Config{
public:
    static Config* get_instance(){
        static Config config;
        return &config;
    }

    void parse(int argc, char* argv[]);

    void set_name(std::string name) {m_name = name;}

    void set_passwd(std::string passwd) {m_passwd = passwd;}

    void set_database_name(std::string database_name) {m_database_name = database_name;}

    const std::string& get_name() const {return m_name;}

    const std::string& get_passwd() const {return m_passwd;}

    const std::string& get_database_name() const {return m_database_name;}

    int get_thread_num() const {return m_thread_num;}

    int get_sql_pool_num() const {return m_sql_pool_num;}

    int get_epoll_trig_mode() const {return m_epoll_trig_mode;}

    int get_port() const {return m_port;}

    bool get_linger() const {return m_linger;}

private:
    Config();

private:
    std::string m_name;
    std::string m_passwd;
    std::string m_database_name;
    int m_thread_num;
    int m_sql_pool_num;
    int m_epoll_trig_mode;
    int m_port;
    bool m_linger;
};


Config::Config(){

    // 线程数量
    m_thread_num = 8;

    // 数据库连接池数量
    m_sql_pool_num = 8;

    // epoll触发模式
    // 0-水平触发 1-边缘触发
    m_epoll_trig_mode = 1;

    // 端口
    m_port = 9006;

    // 是否长连接
    m_linger = true;
}


void
Config::parse(int argc, char* argv[]){
    int opt;
    const char *str = "p:e:l:s:t:";
    while((opt = getopt(argc, argv, str)) != -1){
        switch (opt){
            case 'p':
            {
                m_port = atoi(optarg);
                break;
            }
            case 'e':
            {
                m_epoll_trig_mode = atoi(optarg);
                break;
            }
            case 'l':
            {
                m_linger = atoi(optarg);
                break;
            }
            case 's':
            {
                m_sql_pool_num = atoi(optarg);
                break;
            }
            case 't':
            {
                m_thread_num = atoi(optarg);
                break;
            }
            default:
            {
                printf("not opt\n");
                break;
            }
        }
    }
}

};