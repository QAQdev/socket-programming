#include "util.hh"
class tmp {
public:
    __attribute__((unused)) long type;
    char data[BUFSIZE - 2];
};

std::mutex mtx;
std::condition_variable cr;
std::list<tmp> msg_lst;

class Client{
private:
    sockaddr_in _server_address{};
    pthread_t connected_thread{};
    int _socket_fd = -1; // socket file descriptor
    int msgQueueID;
    __attribute__((unused)) static int getChoice() {
        int choice;
        std::cout << "\n\nPlease choose from:\n"
                "1.connect\n"
                "2.disconnect\n"
                "3.get time\n"
                "4.get server name\n"
                "5.get client list\n"
                "6.send data to other client\n"
                "7.exit\n"
                "> ";
        std::cin >> choice;
        return choice;
    }

    [[noreturn]] static void *thread_handler(void *input){
        int fileDescriptor = *(int *)input;
        char buffer[BUFSIZE];
        recv(fileDescriptor, buffer, BUFSIZE, 0);
        std::cout << buffer << std::endl;
        std::cout << "> ";
        fflush(stdout);
        key_t key = ftok("/", 'a');
        int msgQueueID = msgget(key, IPC_CREAT | 0666);
        if(msgQueueID == -1){
            std::cout << "[Error] msgget failed, error no is " << errno << std::endl;
        }
        while(true) {
            memset(buffer, 0, BUFSIZE);
            auto len = recv(fileDescriptor, buffer, BUFSIZE, 0);
            if (len <= 0) continue;
            std::unique_lock<std::mutex> lck(mtx);
            tmp t{};
            if (buffer[0] == FORWORD) {//REPOST
                std::cout << std::endl << "You received: " << buffer + 1 << std::endl << "> ";
                fflush(stdout);
                continue;
            }

            t.type = static_cast<unsigned char >(buffer[0]);
            memcpy(t.data, buffer + 1, len - 1);
            msgsnd(msgQueueID, &t, BUFSIZE, 0);
        }
    }

    static void printInfo(const std::string& info) {
        std::cout << info << std::endl;
    }

    static void printError(const std::string& err) {
        std::cerr << err << std::endl;
    }

    bool isConnectionExists() const {
        return _socket_fd != -1;
    }

public:
    Client() {
        _socket_fd = -1;
        key_t k = ftok("/", 'a');
        msgQueueID = msgget(k, IPC_CREAT | 0666);
        char msg[BUFSIZE];
        while(msgrcv(msgQueueID, &msg, BUFSIZE, 0, IPC_NOWAIT) > 0){
        }
    }
    ~Client() {
        if (isConnectionExists()) {
            char fin = static_cast<char >(DISCONNECT);
            send(_socket_fd, &fin, sizeof fin, 0);
            printInfo("connection is closed!");
            close(_socket_fd);
        }
    }
    void run() {
        while (true) {
            int choice = getChoice();
            if (choice != 1 && choice != 7 && not isConnectionExists()) {
                printError("NO CONNECTION!");
                continue;
            }
            switch (choice) {
                case 1:{ // connect
                    if (!isConnectionExists()) {
                        _socket_fd = socket(AF_INET, SOCK_STREAM, 0);
                    }
                    std::string ip;
                    int port;
                    ip = "192.168.122.66";
                    port = 5708;

                    _server_address.sin_family = AF_INET;
                    _server_address.sin_port = htons(port);
//                    _server_address.sin_addr.s_addr = inet_addr(ip.c_str());
                    _server_address.sin_addr.s_addr = htonl(INADDR_ANY);
                    std::cout << _socket_fd << std::endl;
                    if (connect(_socket_fd, (struct sockaddr*)&_server_address, sizeof(struct sockaddr)) == -1) {
                        printError("建立连接失败");
                        close(_socket_fd);
                        _socket_fd = -1;
                        break;
                    }
                    pthread_create(&connected_thread, nullptr, thread_handler, &_socket_fd);
                    break;
                }
                case 2: { //disconnect
                    if (not isConnectionExists()) {
                        printError("no socket connection!");
                        break;
                    }
                    char fin = static_cast<char >(DISCONNECT);
                    if (send(_socket_fd, &fin, sizeof fin, 0) <= 0) {
                        printError("cannot send final signal");
                        break;
                    }
                    pthread_cancel(connected_thread);
                    close(_socket_fd);
                    _socket_fd = -1;
                    printInfo("disconnect");
                    break;
                }
                case 3: { //get time
                    char get_time = static_cast<char >(PacketType::GET_TIME);
                    send(_socket_fd, &get_time, sizeof get_time, 0);
                    tmp t{};
                    if(msgrcv(msgQueueID, &t, BUFSIZE, PacketType::GET_TIME, 0) < 0){
                        std::cout << "[Error] msgrcv failed, error no is " << errno << std::endl;
                    }
                    std::cout << "获取时间：" << t.data << std::endl;
                    break;
                }
                case 4: {
                    char get_name = static_cast<char >(PacketType::GET_NAME);
                    send(_socket_fd, &get_name, sizeof get_name, 0);
                    tmp t{};
                    if(msgrcv(msgQueueID, &t, BUFSIZE, PacketType::GET_NAME, 0) < 0){
                        std::cout << "[Error] msgrcv failed, error no is " << errno << std::endl;
                    }
                    std::cout << "获取服务器名称：" << t.data << std::endl;
                    break;
                }
                case 5: {
                    char get_list = static_cast<char >(PacketType::GET_ACTIVE_LIST);
                    send(_socket_fd, &get_list, sizeof get_list, 0);
                    tmp t{};
                    if(msgrcv(msgQueueID, &t, BUFSIZE, PacketType::GET_ACTIVE_LIST, 0) < 0){
                        std::cout << "[Error] msgrcv failed, error no is " << errno << std::endl;
                    }
                    std::cout << "获取用户列表：" << t.data << std::endl;
                    break;
                }
                case 6: {
                    char buffer[BUFSIZE];
                    std::string ip;
                    int port;
                    printInfo("input IP");
                    std::cin >> ip;
                    printInfo("input port");
                    std::cin >> port;
                    buffer[0] = SEND_MSG;
                    printInfo("请输入要发送的信息，#结束");
                    sprintf(buffer + 1, "%s:%d:\n", ip.c_str(), port);
                    char c = ' ';
                    while(c != '#') {
                        std::cin >> c;
                        sprintf(buffer + strlen(buffer), "%c", c);
                    }
                    std::cout << buffer << std::endl;
                    send(_socket_fd, &buffer, sizeof buffer, 0);
                    std::cout << "发送信息：" << std::endl;
                    tmp t{};
                    if(msgrcv(msgQueueID, &t, BUFSIZE, PacketType::SEND_MSG, 0) < 0){
                        std::cout << "[Error] msgrcv failed, error no is " << errno << std::endl;
                    }
                    std::cout << "[Server] " << t.data << std::endl;
                    break;
                }
                case 7: {
                    if (isConnectionExists()) {
                        char fin = static_cast<char >(DISCONNECT);
                        send(_socket_fd, &fin, sizeof fin, 0);
                        close(_socket_fd);
                    }
                    return;
                }
                default:{
                    std::cout << "不合法的选项。请重新输入" << std::endl;
                }
            }
        }
    }
};

int main() {
    auto* client = new Client();
    client->run();
    delete client;
}
