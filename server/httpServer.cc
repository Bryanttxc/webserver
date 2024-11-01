#include <iostream>

#include "httpServer.hh"
#include "http_conn.hh"
#include "../fiberLibrary/hook.hh"

namespace bryant{

http_conn * users;   //用户http数组

http_server::http_server(bool keepalive):
    m_isKeepalive(keepalive) {

    users = new http_conn[MAX_FD];

    //获取当前文件路径
    char server_path[200];
    getcwd(server_path, 200);
    
    // 设置根路径
    char root[6] = "/root"; 
    m_root = (char *)malloc(strlen(server_path) + strlen(root) + 1);
    
    // m_root获得完整路径
    strcpy(m_root, server_path);
    strcat(m_root, root);
    //共享同一个文件描述符（为了防止多定理这个epoll实例，我们只会在协程库中做操作）
}


http_server::~http_server(){
    if(m_root)
        delete m_root;
}


void 
http_server::initmysql_result(Connection_pool *conn_pool) {
    users->initmysql_result(conn_pool);
}


void http_server::handleClient(sylar::Socket::ptr client) {

    // LOG_INFO("[HttpServer] http_server handleClient and dealing %d now", client->getSocket());
    
    bryant::set_hook_enable(true);
    int client_socket = client->getSocket();
    if(!client->isValid()){
        client->close();
        return;
    }

    users[client_socket].init(client, m_root, 0, 
                                m_user, m_password, m_database_name, 
                                m_isKeepalive);

    while( client->isConnected() ){
        // 先读
        if(users[client_socket].read_once()==false) {
            goto end;
        }
        // LOG_INFO("[HttpServer] %s\n", users[client_socket].getReadBuf());
        
        {
            connectionRAII mysqlcon(&users[client_socket].mysql, Connection_pool::get_instance());
        }

        // 处理（process->read, process->write)
        if(users[client_socket].process() == false) {
            goto end;
        }

        // 再写
        if(users[client_socket].write() == false) {
            goto end;
        }
    }
end :
    // LOG_INFO("[HttpServer] %d is close", client->getSocket());
    client->close();
}

} // namespace bryant