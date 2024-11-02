#include <limits.h>

#include "socket.hh"
#include "../fiberLibrary/iomanager.hh"
#include "../fiberLibrary/fdmanager.hh"
#include "../fiberLibrary/hook.hh"

namespace sylar {

//    sock->newSock();
//    sock->m_isConnected = true;
// 可以发现这两个只有在创建udp的时候才有效

Socket::ptr 
Socket::CreateTCP(Address::ptr address) {
    Socket::ptr sock(new Socket(address->getFamily(), TCP, 0) );
    return sock;
}


Socket::ptr 
Socket::CreateUDP(Address::ptr address) {
    Socket::ptr sock(new Socket(address->getFamily(), UDP, 0));
    sock->newSock();
    sock->m_isConnected = true;
    return sock;
}


Socket::ptr 
Socket::CreateTCPSocket() {
    Socket::ptr sock(new Socket(IPv4, TCP, 0));
    return sock;
}


Socket::ptr 
Socket::CreateUDPSocket() {
    Socket::ptr sock(new Socket(IPv4, UDP, 0));
    sock->newSock();
    sock->m_isConnected = true;
    return sock;
}


Socket::ptr 
Socket::CreateTCPSocket6() {
    Socket::ptr sock(new Socket(IPv6, TCP, 0));
    return sock;
}


Socket::ptr 
Socket::CreateUDPSocket6() {
    Socket::ptr sock(new Socket(IPv6, UDP, 0));
    sock->newSock();
    sock->m_isConnected = true;
    return sock;
}


Socket::ptr 
Socket::CreateUnixTCPSocket() {
    Socket::ptr sock(new Socket(UNIX, TCP, 0));
    return sock;
}


Socket::ptr 
Socket::CreateUnixUDPSocket() {
    Socket::ptr sock(new Socket(UNIX, UDP, 0));
    return sock;
}


Socket::Socket(int family, int type, int protocol)
    :m_sock(-1)
    ,m_family(family)
    ,m_type(type)
    ,m_protocol(protocol)
    ,m_isConnected(false) {
}


Socket::~Socket() {
    close();
}


int64_t 
Socket::getSendTimeout() {
    bryant::FdCtx::ptr ctx = bryant::FdMgr::GetInstance()->get(m_sock);
    if(ctx) {
        return ctx->getTimeout(SO_SNDTIMEO);
    }
    return -1;
}


void 
Socket::setSendTimeout(int64_t v) {
    struct timeval tv{int(v / 1000), int(v % 1000 * 1000)};
    setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
}


int64_t 
Socket::getRecvTimeout() {
    bryant::FdCtx::ptr ctx = bryant::FdMgr::GetInstance()->get(m_sock);
    if(ctx) {
        return ctx->getTimeout(SO_RCVTIMEO);
    }
    return -1;
}


void 
Socket::setRecvTimeout(int64_t v) {
    struct timeval tv{int(v / 1000), int(v % 1000 * 1000)};
    setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
}


bool 
Socket::getOption(int level, int option, void* result, socklen_t* len) {
    int rt = getsockopt(m_sock, level, option, result, (socklen_t*)len);
    if(rt) {
        LOG_INFO("[Socket] getOption sock=%d level=%d, option=%d, errno=%d, errstr=%s",
                m_sock, level, option, errno, strerror(errno));
        return false;
    }
    return true;
}


bool 
Socket::setOption(int level, int option, const void* result, socklen_t len) {
    if(setsockopt(m_sock, level, option, result, (socklen_t)len)) {
        LOG_INFO("[Socket] setOption sock=%d level=%d option=%d, errno=%d, errstr=%s",
                m_sock, level, option, errno, strerror(errno));
        return false;
    }
    return true;
}


Socket::ptr 
Socket::accept() {
    Socket::ptr sock(new Socket(m_family, m_type, m_protocol));
    int newsock = ::accept(m_sock, nullptr, nullptr);
    if(newsock == -1) {
        LOG_INFO("[Socket] accept(%d) errno=%d errstr=%s",
                 m_sock, errno, strerror(errno));
        return nullptr;
    }
    if(sock->init(newsock)) {
        return sock;
    }
    return nullptr;
}


bool 
Socket::init(int sock) {
    //从单例类中获取这个文件描述符的信息(注意返回的是一个刚初始化的智能指针，固然会指向相同的地方，且默认isSocket，isClose都是false，固然成功进入）
    bryant::FdCtx::ptr ctx = bryant::FdMgr::GetInstance()->get(sock);
    if(ctx && ctx->isSocket() && !ctx->isClosed()) {
        m_sock = sock;
        m_isConnected = true;
        initSock();
        getLocalAddress();
        getRemoteAddress();
        return true;
    }
    return false;
}

bool 
Socket::bind(const Address::ptr addr) {
//    m_localAddress = addr;
    if(!isValid()) {
        newSock();
    }

    //绑定的时候，地址这些都需要保证一样
    if(addr->getFamily() != m_family) {
        LOG_INFO("[Socket] Socket::bind addr->getFamily() != m_family error");
        return false;
    }
    //UnixAddress转化成这个查看是否可以成功，如果成功，就说明关注UnixAddress，从而做专门的UnixAddress操作

    UnixAddress::ptr uaddr = std::dynamic_pointer_cast<UnixAddress>(addr);
    if(uaddr) {
        Socket::ptr sock = Socket::CreateUnixTCPSocket();
        if(sock->connect(uaddr)) {
            return false;
        } else {
//            bryant::FSUtil::Unlink(uaddr->getPath(), true);
        }
    }

    if(::bind(m_sock, addr->getAddr(), addr->getAddrLen())) {
        LOG_INFO("[Socket] Socket::bind(m_sock, addr->getAddr(), addr->getAddrLen()) error");
        return false;
    }
    getLocalAddress();
    return true;
}


bool 
Socket::reconnect(uint64_t timeout_ms) {
    if(!m_remoteAddress) {
        return false;
    }
    m_localAddress.reset();
    return connect(m_remoteAddress, timeout_ms);
}


bool 
Socket::connect(const Address::ptr addr, uint64_t timeout_ms) {
    m_remoteAddress = addr;
    if(!isValid()) {
        newSock();
        if(!isValid() ) {
            return false;
        }
    }

    if(addr->getFamily() != m_family) {
        return false;
    }
    //说明没有设置等待时长，直接进行阻塞等待
    if(timeout_ms == (uint64_t)-1) {
        if(::connect(m_sock, addr->getAddr(), addr->getAddrLen())) {
            close();
            return false;
        }
    } else {
        //否则阻塞等待这么长的时间
        if(::connect_with_timeout(m_sock, addr->getAddr(), addr->getAddrLen(), timeout_ms)) {
            close();
            return false;
        }
    }
    m_isConnected = true;
    getRemoteAddress();
    getLocalAddress();
    return true;
}


bool 
Socket::listen(int backlog) {
    if(!isValid()) {
        return false;
    }
    if(::listen(m_sock, backlog)) { //函数中用于设置连接队列的最大长度,设置为backlog
        return false;
    }
    return true;
}


bool 
Socket::close() {
    if(!m_isConnected && m_sock == -1) {
        return true;
    }
    m_isConnected = false;
    if(m_sock != -1) {
        ::close(m_sock);
        m_sock = -1;
    }
    return false;
}


//自己多加的函数
ssize_t 
Socket::writev(const struct iovec *iov, int iovcnt){
    if(isConnected()) {
        return ::writev(m_sock,iov,iovcnt);
    }
    return -1;
}


int 
Socket::send(const void* buffer, size_t length, int flags) {
    if(isConnected()) {
        return ::send(m_sock, buffer, length, flags);
    }
    return -1;
}


int 
Socket::send(const iovec* buffers, size_t length, int flags) {
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        return ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}


int 
Socket::sendTo(const void* buffer, size_t length, const Address::ptr to, int flags) {
    if(isConnected()) {
        return ::sendto(m_sock, buffer, length, flags, to->getAddr(), to->getAddrLen());
    }
    return -1;
}


int 
Socket::sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags) {
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        msg.msg_name = to->getAddr();
        msg.msg_namelen = to->getAddrLen();
        return ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}


int 
Socket::recv(void* buffer, size_t length, int flags) {
    if(isConnected()) {
        return ::recv(m_sock, buffer, length, flags);
    }
    return -1;
}


int 
Socket::recv(iovec* buffers, size_t length, int flags) {
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        return ::recvmsg(m_sock, &msg, flags);
    }
    return -1;
}


int 
Socket::recvFrom(void* buffer, size_t length, Address::ptr from, int flags) {
    if(isConnected()) {
        socklen_t len = from->getAddrLen();
        return ::recvfrom(m_sock, buffer, length, flags, from->getAddr(), &len);
    }
    return -1;
}


int 
Socket::recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags) {
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        msg.msg_name = from->getAddr();
        msg.msg_namelen = from->getAddrLen();
        return ::recvmsg(m_sock, &msg, flags);
    }
    return -1;
}


Address::ptr 
Socket::getRemoteAddress() {
    if(m_remoteAddress) {
        return m_remoteAddress;
    }

    Address::ptr result;
    switch(m_family) {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnknownAddress(m_family));
            break;
    }
    socklen_t addrlen = result->getAddrLen();
    if(getpeername(m_sock, result->getAddr(), &addrlen)) {
        return Address::ptr(new UnknownAddress(m_family));
    }
    if(m_family == AF_UNIX) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrLen(addrlen);
    }
    m_remoteAddress = result;
    return m_remoteAddress;
}


Address::ptr 
Socket::getLocalAddress() {
    if(m_localAddress) {
        return m_localAddress;
    }

    Address::ptr result;
    switch(m_family) {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnknownAddress(m_family));
            break;
    }
    socklen_t addrlen = result->getAddrLen();
    if(getsockname(m_sock, result->getAddr(), &addrlen)) {
        return Address::ptr(new UnknownAddress(m_family));
    }
    if(m_family == AF_UNIX) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrLen(addrlen);
    }
    m_localAddress = result;
    return m_localAddress;
}


bool 
Socket::isValid() const {
    return m_sock != -1;
}


int 
Socket::getError() {
    int error = 0;
    socklen_t len = sizeof(error);
    if(!getOption(SOL_SOCKET, SO_ERROR, &error, &len)) {
        error = errno;
    }
    return error;
}


std::ostream& 
Socket::dump(std::ostream& os) const {
    os << "[Socket sock=" << m_sock
       << " is_connected=" << m_isConnected
       << " family=" << m_family
       << " type=" << m_type
       << " protocol=" << m_protocol;
    if(m_localAddress) {
        os << " local_address=" << m_localAddress->toString();
    }
    if(m_remoteAddress) {
        os << " remote_address=" << m_remoteAddress->toString();
    }
    os << "]";
    return os;
}


std::string 
Socket::toString() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}


bool 
Socket::cancelRead() {
    return bryant::IOManager::GetThis()->cancelEvent(m_sock, bryant::IOManager::READ);
}


bool 
Socket::cancelWrite() {
    return bryant::IOManager::GetThis()->cancelEvent(m_sock, bryant::IOManager::WRITE);
}


bool 
Socket::cancelAccept() {
    return bryant::IOManager::GetThis()->cancelEvent(m_sock, bryant::IOManager::READ);
}


bool 
Socket::cancelAll() {
    return bryant::IOManager::GetThis()->cancelAll(m_sock);
}


void 
Socket::initSock() {
    int val = 1;
    setOption(SOL_SOCKET, SO_REUSEADDR, val);
    if(m_type == SOCK_STREAM) {
        setOption(IPPROTO_TCP, TCP_NODELAY, val);
    }
}


void 
Socket::newSock() {
    m_sock = socket(m_family, m_type, m_protocol);
    if(m_sock != -1) {
        initSock();
    } else {
        //输出日志
        LOG_INFO("[Socket] newSock() failed");
    }

}

std::ostream& operator<<(std::ostream& os, const Socket& sock) {
    return sock.dump(os);
}

}