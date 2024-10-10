#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "hook.hh"
#include "iomanager.hh"

int fd = 0;

void test_fiber(){
    LOG_INFO("test_fiber");

    // step1: create socket
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1) {
        LOG_ERROR("socket error, errno=%d, errno_str=%s", errno, strerror(errno));
    }

    // step2: set nonblock and create socketaddr
    fcntl(fd, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    addr.sin_port = htons(80);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("153.3.238.102");

    // step3: connect
    if(!connect(fd, (const sockaddr*)&addr, sizeof(addr))){
        ;
    } else if(errno == EINPROGRESS){
        LOG_DEBUG("connect in progress");

        bryant::IOManager::GetThis()->addEvent(fd, bryant::IOManager::READ, [](){
            LOG_INFO("read success");
        });

        bryant::IOManager::GetThis()->addEvent(fd, bryant::IOManager::WRITE, [](){
            LOG_INFO("write success");
            bryant::IOManager::GetThis()->cancelEvent(fd, bryant::IOManager::READ);
            close(fd);
        });
    } else {
        LOG_ERROR("connect error");
    }
}


void test_iomanager(){
    bryant::IOManager iom(2, false);
    iom.schedule(&test_fiber);
}


int main(){
    test_iomanager();
    return 0;
}