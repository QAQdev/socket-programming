#include <iostream>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <mutex>
#include <cstring>
#include <vector>
#include <map>
#include <pthread.h>
#include <list>
#include <condition_variable>
constexpr int BUFSIZE = 256;

using fd_t  = int; // 文件描述符类型
using port_t = int; // 端口数据类型
using inet_t = std::string; // 点分十进制类型
using client_t = std::pair<inet_t,port_t>; // 客户端类型

enum PacketType {
    CONNECT = 1,
    DISCONNECT,
    GET_TIME,
    GET_NAME,
    GET_ACTIVE_LIST,
    SEND_MSG,
    EXIT,
    FORWORD
};
