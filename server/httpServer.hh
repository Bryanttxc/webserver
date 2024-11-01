#pragma once

#include <string>

#include "tcpServer.hh"
#include"../CGImysql/sql_connection_pool.hh"
#include"../util/socket.hh"

namespace bryant{

const int MAX_FD = 65536;

class http_server : public TcpServer {
public:
    /**
     * @brief Construct a new http server
     * 
     * @param keepalive 
     */
    http_server(bool keepalive = true); //默认长连接
   
    /**
     * @brief Destroy the http server
     * 
     */
    ~http_server();

    //这两个是间接给到http的
    /**
     * @brief initiate mysql
     * 
     * @param conn_pool 
     */
    void initmysql_result(Connection_pool *conn_pool);

    /**
     * @brief handle client mission
     * 
     * @param client 
     */
    void handleClient(sylar::Socket::ptr client) override;

public:
    char* m_root = nullptr;
    std::string m_user;
    std::string m_password;
    std::string m_database_name;

private:
    bool m_isKeepalive;
};

}