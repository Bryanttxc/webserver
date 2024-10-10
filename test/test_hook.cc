#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <iostream>

#include "hook.hh"
#include "iomanager.hh"

void test_sleep(){
    bryant::IOManager iom(1);

    iom.schedule([](){
        sleep(2);
        LOG_INFO("sleep 2");
    });

    iom.schedule([](){
        sleep(3);
        LOG_INFO("sleep 3");
    });

    LOG_INFO("test sleep");
}

int fd = 0;

// 模拟客户端与指定服务器通信
// 调用socket相关函数，确认hook是否生效
void test_sock(){

    bryant::IOManager iom(2, false);

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1){
        LOG_ERROR("socket create error");
    }

    sockaddr_in addr;
    memset(&addr, '\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    addr.sin_addr.s_addr = inet_addr("153.3.238.102");

    int rt = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    if(rt){ // errno
        return;
    }

    std::string message = "GET / HTTP/1.0\r\n\r\n";
    rt = send(fd, message.c_str(), message.size(), 0);
    if(rt == -1){
        return;
    }

    std::string buf;
    buf.resize(4096);
    rt = recv(fd, (void*)buf.data(), buf.size(), 0);
    if(rt == -1){
        return;
    }

    buf.resize(rt);
    std::cout << buf << std::endl;
}


int main(){
    // test_sleep();
    test_sock();
    return 0;
}