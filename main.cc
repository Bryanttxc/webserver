#include <string>

#include "../util/config.hh"
#include "../log/logger.hh"
#include "../fiberLibrary/iomanager.hh"
#include "../server/httpServer.hh"


bryant::IOManager::ptr worker = nullptr;

void run(){
    bryant::Config* config = bryant::Config::get_instance();

    // 初始化日志
    bryant::Logger::getInstance()->init("webServerLogger");

    // 初始化SQL
    bryant::Connection_pool* conn_pool = bryant::Connection_pool::get_instance();
    conn_pool->init("localhost", config->get_name(), config->get_passwd(), config->get_database_name(), config->get_port(), config->get_sql_pool_num());

    // 初始化IOManager
    worker.reset(new bryant::IOManager(bryant::Config::get_instance()->get_thread_num(), false));

    // 初始化httpServer
    std::shared_ptr<bryant::http_server> httpServer = std::make_shared<bryant::http_server>(config->get_linger());
    httpServer->initmysql_result(bryant::Connection_pool::get_instance());
    httpServer->m_user = config->get_name();
    httpServer->m_password = config->get_passwd();
    httpServer->m_database_name = config->get_database_name();

    sylar::Address::ptr m_address = sylar::Address::LookupAnyIPAddress("127.0.0.1:" + std::to_string(config->get_port()) );
    while(!httpServer->bind(m_address,false)){
        sleep(1);
    }
    LOG_INFO("[Server] http start!!!\n");
    httpServer->start();
}


int main(int argc, char* argv[]){
    // 输入sql账号密码
    std::string username = "root";
    std::string password = "root";
    std::string database_name = "bryant_webserver";

    // 解析命令行参数
    bryant::Config::get_instance()->set_name(username);
    bryant::Config::get_instance()->set_passwd(password);
    bryant::Config::get_instance()->set_database_name(database_name);
    bryant::Config::get_instance()->parse(argc, argv);

    // 启动服务器所有模块
    LOG_INFO("[Server] run server!\n");
    bryant::IOManager manager(1);
    manager.schedule([]() {run();});

    return 0;
}