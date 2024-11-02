#pragma once

#include <cstdio>
#include <cstring>
#include <error.h>
#include <string>
#include <list>
#include <iostream>
#include <mysql/mysql.h>

#include "../util/locker.hh"
#include "../log/logger.hh"

namespace bryant{

class Connection_pool {
private:
    Connection_pool() = default; //单例模式
    ~Connection_pool();

    int m_max_conn;      //最大连接数
    int m_cur_conn = 0;  //当前已经使用的连接数
    int m_free_conn = 0; //当前空闲的连接数
    Mutex lock;

    std::list<MYSQL*> conn_list; //连接池
    Semaphore reserve;           //信号量

public:
    std::string m_url;          //主机地址
    std::string m_user;         //登录数据库的用户名
    std::string m_passwd;       //登录数据库的密码
    std::string m_databasename; //数据库名
    int m_port;                 //数据库端口号

public:
    MYSQL*get_connection();             //获取数据库连接
    bool release_connection(MYSQL*conn);//释放连接
    int get_free_conn();                //获取连接
    void destroy_pool();                //销毁所有连接

    static Connection_pool *get_instance();
	void init(std::string url, std::string user, 
            std::string passwd, std::string data_base_name,
			int port, int max_conn);
};


//这个类主要是体现了RAII思想的，创建一个中间类，
//使用者可以不用关心数据库连接池的底层细节
class connectionRAII{
public:
	connectionRAII(MYSQL **con, Connection_pool *connPool);
	~connectionRAII();
	
private:
	MYSQL *conRAII;
	Connection_pool *poolRAII;
};

} // namespace bryant