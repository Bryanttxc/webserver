#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "fdmanager.hh"
#include "hook.hh"

namespace bryant{

// instantiate
template class Singleton<FdManager>;

// Static variables need to be defined outside the class
template<typename T>
T* Singleton<T>::instance = nullptr;

template<typename T>
Mutex Singleton<T>::mutex;	


FdCtx::FdCtx(int fd)
    :m_fd(fd) {
	init();
}


FdCtx::~FdCtx() {
}


bool 
FdCtx::init() {
	if(m_isInit) {
		return true;
	}
	
	struct stat statbuf;
	if(-1 == fstat(m_fd, &statbuf)) { // fd is in valid
		m_isInit = false;
		m_isSocket = false;
	} else {
		m_isInit = true;	
		m_isSocket = S_ISSOCK(statbuf.st_mode);	
	}

	if(m_isSocket) { // if it is a socket -> set to nonblock
		// fcntl_f() -> the original fcntl() -> get the socket info
		int flags = fcntl_f(m_fd, F_GETFL, 0);
		if( !(flags & O_NONBLOCK) ) {
			// if not -> set to nonblock
			fcntl_f(m_fd, F_SETFL, flags | O_NONBLOCK);
		}
		m_sysNonblock = true;
	}
	else {
		m_sysNonblock = false;
	}

	return m_isInit;
}


void
FdCtx::setTimeout(int type, uint64_t v) {
	if(type == SO_RCVTIMEO) {
		m_recvTimeout = v;
	}
	else {
		m_sendTimeout = v;
	}
}


uint64_t
FdCtx::getTimeout(int type) {
	if(type == SO_RCVTIMEO) {
		return m_recvTimeout;
	}
	else {
		return m_sendTimeout;
	}
}


FdManager::FdManager() {
	m_fds.resize(64);
}


std::shared_ptr<FdCtx>
FdManager::get(int fd, bool auto_create) {
	if(fd == -1) {
		return nullptr;
	}

	m_rwmutex.readLock();
	if(m_fds.size() <= fd) {
		if(auto_create == false) {
			m_rwmutex.unlock();
			return nullptr;
		}
	} 
	else {
		if(m_fds[fd] || !auto_create) {
			m_rwmutex.unlock();
			return m_fds[fd];
		}
	}
	m_rwmutex.unlock();

	// case: auto_create == true
	m_rwmutex.writeLock();
	if(m_fds.size() <= fd) {
		m_fds.resize(fd * 1.5);
	}
	m_fds[fd] = std::make_shared<FdCtx>(fd);
    m_rwmutex.unlock();

	return m_fds[fd];
}


void FdManager::del(int fd)
{
	m_rwmutex.writeLock();
	if(m_fds.size() <= fd){
		return;
	}
	m_fds[fd].reset();
    m_rwmutex.unlock();
}

}