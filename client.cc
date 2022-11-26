#include "util.hh"

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
    static void *thread_handler(void *input) {
        return nullptr;
    }

public:
    Client() {

    }
    void run() {
        while (true) {
            int choice = getChoice();
            switch (choice) {
                case 1:{ // connect
                    std::string ip;
                    int port;
                    std::cout << "input IP" << std::endl;
                    std::cin >> ip;
                    std::cout << "input port number" << std::endl;
                    std::cin >> port;
                    _socket_fd = socket(AF_INET, SOCK_STREAM, 0);
                    _server_address.sin_family = AF_INET;
                    _server_address.sin_port = htons(port);
                    _server_address.sin_addr.s_addr = htonl(INADDR_ANY);
                    auto res = connect(_socket_fd, (struct sockaddr*)&_server_address, sizeof(struct sockaddr));
                    pthread_create(&connected_thread, nullptr, thread_handler, &_socket_fd);
                    break;
                }
                case 2: {

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
    client.run();
    return 0;
}