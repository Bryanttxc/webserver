#include <cstring>

#include "../util/socket.hh"
#include "../fiberLibrary/iomanager.hh"
#include "../log/logger.hh"


// 模拟客户端向服务端发送数据
void client(){
    sylar::Address::ptr address = sylar::Address::LookupAny("www.baidu.com:80");
    if(!address){
        LOG_ERROR("Address not found");
    } else {
        LOG_INFO("Address: %s", address->toString().c_str());
    }
    
    sylar::Socket::ptr sock = sylar::Socket::CreateTCP(address);
    if(!sock->connect(address)){
        LOG_INFO("connect %s fail", address->toString().c_str());
    } else {
        LOG_INFO("connect %s success", address->toString().c_str());
    }
    
    uint64_t ts = bryant::GetCurrentUS();
    for(long long i = 0; i < 10000000000ul; ++i){
        if(int err = sock->getError()){
            LOG_INFO("err=%d errstr=%s", err, strerror(err));
            break;
        }

        static int batch = 10000000;
        if(i && (i % batch) == 0){
            uint64_t now = bryant::GetCurrentUS();
            printf("i=%lld, cost %lu us\n", i, now - ts);
            ts = now;
        }
    }
}


void test_socket(){
    bryant::IOManager iom(1);
    iom.schedule(&client);
}


int main(){
    test_socket();
    return 0;
}