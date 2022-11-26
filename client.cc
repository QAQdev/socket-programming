#include "util.hh"
class tmp {
public:
    long type;
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

    static void *thread_handler(void *input){
        int fileDescriptor = *(int *)input;
        tmp t{};
        char buffer[BUFSIZE];
        recv(fileDescriptor, buffer, BUFSIZE, 0);
        while(true){
            memset(buffer, 0, BUFSIZE);
            auto len = recv(fileDescriptor, buffer, BUFSIZE, 0);
            std::unique_lock<std::mutex> lck(mtx);

            if(buffer[0] == PacketType::EXIT){
                break;
            }
            if(buffer[0] == 114) {//REPOST
                std::cout << buffer << std::endl;
            }
            t.type = static_cast<unsigned char >(buffer[0]);
            memcpy(t.data, buffer + 1, len - 1);
            msg_lst.push_back(t);
            cr.notify_all();
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
    Client() = default;
    void Init() {
        _socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (_socket_fd == -1) {
            throw std::exception();
        }
    }
    [[noreturn]] void run() {
        while (true) {
            int choice = getChoice();
            switch (choice) {
                case 1:{ // connect
                    if (isConnectionExists()) {
                        printInfo("a socket connection has been built!");
                    }
                    std::string ip;
                    int port;
                    printInfo("input IP");
                    std::cin >> ip;
                    printInfo("input port");
                    std::cin >> port;
                    _server_address.sin_family = AF_INET;
                    _server_address.sin_port = htons(port);
                    _server_address.sin_addr.s_addr = htonl(INADDR_ANY);
                    if (connect(_socket_fd, (struct sockaddr*)&_server_address, sizeof(struct sockaddr)) == -1) {
                        printError("建立连接失败");
                        break;
                    }
                    pthread_create(&connected_thread, nullptr, thread_handler, &_socket_fd);
                    break;
                }
                case 2: { //disconnect
                    if (not isConnectionExists()) {
                        printError("no socket connection!");
                    }
                    char fin = static_cast<char >(4); //EOT
                    if (send(_socket_fd, &fin, sizeof fin, 0) <= 0) {
                        printError("cannot send final signal");
                        break;
                    }
                    close(_socket_fd);
                    _socket_fd = -1;
                    printInfo("disconnect");
                    break;
                }
                case 3: { //get time
                    char get_time = static_cast<char >(PacketType::GET_TIME);
                    send(_socket_fd, &get_time, sizeof get_time, 0);
                    while (true) {
                        std::unique_lock<std::mutex> lck(mtx);
                        while (msg_lst.empty()) {
                            cr.wait(lck);
                        }
                        auto tmp = std::move(msg_lst.front());
                        std::cout << "huoqushijian：" << std::endl;
                        msg_lst.pop_front();
                        break;
                    }
                }
                case 4: {
                    char get_time = static_cast<char >(PacketType::GET_NAME);
                    send(_socket_fd, &get_time, sizeof get_time, 0);
                    while (true) {
                        std::unique_lock<std::mutex> lck(mtx);
                        while (msg_lst.empty()) {
                            cr.wait(lck);
                        }
                        auto tmp = std::move(msg_lst.front());
                        std::cout << "huoquname：" << std::endl;
                        msg_lst.pop_front();
                        break;
                    }
                    break;
                }
                case 5: {
                    char get_list = static_cast<char >(PacketType::GET_ACTIVE_LIST);
                    send(_socket_fd, &get_list, sizeof get_list, 0);
                    while (true) {
                        std::unique_lock<std::mutex> lck(mtx);
                        while (msg_lst.empty()) {
                            cr.wait(lck);
                        }
                        auto tmp = std::move(msg_lst.front());
                        std::cout << "huoqushijian：" << std::endl;
                        msg_lst.pop_front();
                        break;
                    }
                }
                case 6: {

                    break;
                }
                case 7: {
                    if (isConnectionExists()) {
                        char fin = static_cast<char >(4);
                        send(_socket_fd, &fin, sizeof fin, 0);
                        close(_socket_fd);
                    }
                    exit(0);
                }
                default:{
                    std::cout << "不合法的选项。请重新输入" << std::endl;
                }
            }
        }
    }
};

int main() {
    Client client;
    try {
        client.Init();
    } catch (std::exception &e) {
        std::cerr << "cannot build a socket connection" << std::endl;
        return 0;
    }
    client.run();
}