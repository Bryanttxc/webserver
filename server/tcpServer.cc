#include "tcpServer.hh"

using namespace std;

namespace bryant {

static uint64_t g_tcp_server_read_timeout = (uint64_t)(60 * 1000 * 2);


//m_isStop=true表明服务还未开启
TcpServer::TcpServer(IOManager* worker,
                    IOManager* io_worker,
                    IOManager* accept_worker)
                    :m_worker(worker),
                    m_ioWorker(io_worker),
                    m_acceptWorker(accept_worker),
                    m_recvTimeout(g_tcp_server_read_timeout),
                    m_name("sylar/1.0.0"),
                    m_isStop(true)   {
}


TcpServer::~TcpServer() {
    for(auto& i : m_socks) {
        i->close();
    }
    m_socks.clear(); // 清空m_socks数组s
}


bool TcpServer::bind(sylar::Address::ptr addr, bool ssl) {
    std::vector<sylar::Address::ptr> addrs;
    std::vector<sylar::Address::ptr> fails;
    addrs.push_back(addr);
    return bind(addrs, fails, ssl);
}


bool TcpServer::bind(const std::vector<sylar::Address::ptr>& addrs,
                           std::vector<sylar::Address::ptr>& fails,
                           bool ssl) {
    m_ssl = ssl;

    for(auto& addr : addrs) {
        sylar::Socket::ptr sock = sylar::Socket::CreateTCP(addr);
        
        // 绑定监听地址
        if(!sock->bind(addr) ) {
            LOG_INFO("[TcpServer] bind fail errno=%d errstr=%s addr=[%s]", 
                    errno, strerror(errno), addr->toString());

            fails.push_back(addr);
            continue;
        }

        // 监听sock
        if(!sock->listen()) {
            LOG_INFO("[TcpServer] listen fail errno=%d errstr=%s addr=[%s]", 
                    errno, strerror(errno), addr->toString());

            fails.push_back(addr);
            continue;
        }
        m_socks.push_back(sock);
    } // end for

    if(!fails.empty()) {
        m_socks.clear();
        return false;
    }

    return true;
}


void TcpServer::startAccept(sylar::Socket::ptr sock) {
    while(!m_isStop) {
        //一个sockfd描述符监听多个client，调用属于每个client独特的handleClient做处理
        sylar::Socket::ptr client = sock->accept();
        if(!client) {
            LOG_INFO("[TcpServer] accept errno=%d errstr=%s", errno, strerror(errno));
            continue;
        }

        client->setRecvTimeout(m_recvTimeout);
        m_ioWorker->schedule(std::bind(&TcpServer::handleClient,
                    shared_from_this(), client));
    }
}


bool TcpServer::start() {
    if(!m_isStop) { // 已经开启
        return true;
    }
    m_isStop = false;

    for(auto& sock : m_socks) {
        m_acceptWorker->schedule(std::bind(&TcpServer::startAccept,
                    shared_from_this(), sock));
    }
    return true;
}


void TcpServer::stop() {
    m_isStop = true;
    auto self = shared_from_this();
    m_acceptWorker->schedule([this, self]() {
        for(auto& sock : m_socks) {
            sock->cancelAll();
            sock->close();
        }
        m_socks.clear();
    });
}


std::string 
TcpServer::toString(const std::string& prefix) {
    std::stringstream ss;
    ss << prefix << "[type=" << m_type
       << " name=" << m_name << " ssl=" << m_ssl
       << " worker=" << (m_worker ? m_worker->getName() : "")
       << " accept=" << (m_acceptWorker ? m_acceptWorker->getName() : "")
       << " recv_timeout=" << m_recvTimeout << "]" << std::endl;
    std::string pfx = prefix.empty() ? "    " : prefix;
    for(auto& i : m_socks) {
        ss << pfx << pfx << *i << std::endl;
    }
    return ss.str();
}


void 
TcpServer::handleClient(sylar::Socket::ptr client) {
    printf("TcpServer handleClient need override !!!\n");
}

}