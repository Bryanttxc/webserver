#pragma once

#include <memory>
#include <vector>

#include "../util/locker.hh"

namespace bryant {

// fd上下文类
// 创建的意义：将fd的状态封装一个类，方便获取
class FdCtx : public std::enable_shared_from_this<FdCtx> {
public:
	using ptr = std::shared_ptr<FdCtx>;

	FdCtx(int fd);
	~FdCtx();

	bool init();
	bool isInit() const {return m_isInit;}
	bool isSocket() const {return m_isSocket;}
	bool isClosed() const {return m_isClosed;}

	void setUserNonblock(bool v) {m_userNonblock = v;}
	bool getUserNonblock() const {return m_userNonblock;}

	void setSysNonblock(bool v) {m_sysNonblock = v;}
	bool getSysNonblock() const {return m_sysNonblock;}

	void setTimeout(int type, uint64_t v);
	uint64_t getTimeout(int type);

private:
	bool m_isInit = false; 			// 是否初始化
	bool m_isSocket = false; 		// 是否是socket
	bool m_sysNonblock = false; 	// 是否系统非阻塞
	bool m_userNonblock = false; 	// 是否用户非阻塞
	bool m_isClosed = false; 		// 是否关闭
	int m_fd; 						// fd

	// read event timeout
	uint64_t m_recvTimeout = (uint64_t)-1;
	// write event timeout
	uint64_t m_sendTimeout = (uint64_t)-1;
};


// fd管理类
class FdManager {
public:
	FdManager();

	/**
	 * @brief get shared_ptr point to fd
	 * @details get the status relevant to socket
	 * 
	 * @param fd 
	 * @param auto_create
	 * @return FdCtx::ptr include the status of fd
	 */
	FdCtx::ptr get(int fd, bool auto_create = false);
	
	/**
	 * @brief delete fd
	 * 
	 * @param fd 
	 */
	void del(int fd);

private:
	RWMutex m_rwmutex;
	std::vector<FdCtx::ptr> m_fds;
};


template<typename T>
class Singleton {
private:
    static T* instance; // 饿汉式
    static Mutex mutex;

protected:
    Singleton() {}  

public:
    // Delete copy constructor and assignment operation
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

    static T* GetInstance() {
        mutex.lock();
        if (instance == nullptr) {
            instance = new T();
        }
		mutex.unlock();
        return instance;
    }

    static void DestroyInstance() {
		mutex.lock();
        delete instance;
        instance = nullptr; // avoid memory leak
		mutex.unlock();
    }
};

using FdMgr = Singleton<FdManager>;

}
