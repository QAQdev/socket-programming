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
#include <pthread.h>

constexpr int BUFSIZE = 256;

enum MsgType
{
    CONNECT = 1,
    DISCONNECT,
    GET_TIME,
    GET_NAME,
    GET_ACTIVE_LIST,
    SEND_MSG,
    EXIT,
    FORWORD
};
