/**
 * @file tcp_server.h
 * @brief TCP服务器的封装
 * @author sylar.yin
 * @email 564628276@qq.com
 * @date 2019-06-05
 * @copyright Copyright (c) 2019年 sylar.yin All rights reserved (www.sylar.top)
 */
#pragma once

#include <memory>
#include <sstream>
#include <iostream>
#include <string>

#include "../fiberLibrary/iomanager.hh"
#include "../util/socket.hh"

namespace bryant {

/**
* @brief TCP服务器封装
* @details 包含服务器与客户端建立连接时的步骤
*/
class TcpServer : public std::enable_shared_from_this<TcpServer> {
public:
    using ptr = std::shared_ptr<TcpServer>;

    /**
    * @brief 构造函数
    * @param[in] worker socket客户端工作的协程调度器
    * @param[in] accept_worker 服务器socket执行接收socket连接的协程调度器
    */
    TcpServer(IOManager* worker = IOManager::GetThis(),
              IOManager* io_woker = IOManager::GetThis(),
              IOManager* accept_worker = IOManager::GetThis());

    /**
    * @brief 析构函数
    */
    virtual ~TcpServer();

    /**
    * @brief 绑定地址
    * @return 返回是否绑定成功
    */
    virtual bool bind(sylar::Address::ptr addr, bool ssl = false);

    /**
    * @brief 绑定地址数组
    * @param[in] addrs 需要绑定的地址数组
    * @param[out] fails 绑定失败的地址
    * @return 是否绑定成功
    */
    virtual bool bind(const std::vector<sylar::Address::ptr>& addrs,
                            std::vector<sylar::Address::ptr>& fails,
                            bool ssl = false);

    /**
    * @brief 启动服务
    * @pre 需要bind成功后执行
    */
    virtual bool start();

    /**
    * @brief 停止服务
    */
    virtual void stop();

    /**
    * @brief 返回读取超时时间(毫秒)
    */
    uint64_t getRecvTimeout() const { return m_recvTimeout;}

    /**
    * @brief 返回服务器名称
    */
    std::string getName() const { return m_name;}

    /**
    * @brief 设置读取超时时间(毫秒)
    */
    void setRecvTimeout(uint64_t v) { m_recvTimeout = v;}

    /**
    * @brief 设置服务器名称
    */
    virtual void setName(const std::string& v) { m_name = v;}

    /**
    * @brief 是否停止
    */
    bool isStop() const { return m_isStop;}

    /**
     * @brief 转字符串
     * 
     * @param prefix 
     * @return std::string 
     */
    virtual std::string toString(const std::string& prefix = "");

    /**
     * @brief 获取Socket数组
     * 
     * @return std::vector<Socket::ptr> 
     */
    std::vector<sylar::Socket::ptr> getSocks() const { return m_socks;}

protected:
    /**
    * @brief 处理新连接的Socket类
    */
    virtual void handleClient(sylar::Socket::ptr client);

    /**
    * @brief 开始接受连接
    */
    virtual void startAccept(sylar::Socket::ptr sock);

protected:
    std::vector<sylar::Socket::ptr> m_socks;   // 监听Socket数组

    IOManager* m_worker;                // 新连接的Socket工作的调度器
    IOManager* m_ioWorker;              // 新连接的Socket工作的调度器
    IOManager* m_acceptWorker;          // 服务器Socket接收连接的调度器

    uint64_t m_recvTimeout;             // 接收超时时间(毫秒)
    std::string m_name;                 // 服务器名称
    std::string m_type = "tcp";         // 服务器类型
    bool m_isStop;                      // 服务器是否停止

    bool m_ssl = false;
};

} // namespace bryant
