#include "util.hh"

class Server {
private:
    fd_t _sock_fd; // 套接字文件描述符
    sockaddr_in _sock_addr_in{}; // 套接字地址
    static constexpr int _max_connection = 20; // 最大连接数
    port_t _default_port = 5708; // 学号后 4 位作为端口号
    std::map<fd_t, client_t> _client_list;
    std::mutex _mtx;

    struct Args {
        fd_t conn_fd;
        std::map<fd_t, client_t> *_client_list;
        std::mutex *_mtx;
    };

private:
    static void *task(void *args) {
        conn_handler(*(Args *) args);
        return nullptr;
    }

public:

    Server() {
        // 建立 socket 连接
        _sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        _sock_addr_in.sin_family = AF_INET;

        // 端口号为学号后四位，IP 地址为本机所有地址
        _sock_addr_in.sin_port = htons(_default_port);
        _sock_addr_in.sin_addr.s_addr = htonl(INADDR_ANY);

        // bind 绑定地址后用 listen 监听
        bind(_sock_fd, (sockaddr *) &_sock_addr_in, sizeof(_sock_addr_in));
        listen(_sock_fd, _max_connection);
    }

    ~Server() {
        close(_sock_fd); // 关闭 socket 连接
    }

    void run() {
        std::cout << "server is running\n";

        while (true) {
            sockaddr_in client{};
            int client_addr_len = sizeof(client);
            // 阻塞式等待连接请求
            int connection_fd = accept(_sock_fd, (sockaddr *) &client, (socklen_t *) &client_addr_len);

            // 将客户端信息加入列表
            _client_list.insert(std::make_pair(
                    connection_fd, client_t(
                            inet_ntoa(client.sin_addr),
                            ntohs(client.sin_port))));

            std::cout << inet_ntoa(client.sin_addr) << ":" << ntohs(client.sin_port) << " connected\n";

            // BIO 模式，为每个客户端建立一个线程监听请求
            pthread_t client_thread;
            Args args = {connection_fd, &_client_list, &_mtx};
            pthread_create(&client_thread, nullptr, task, &args);
        }
    }

    static void conn_handler(Args args) {
        // 向客户端发送一个 hello
        std::string hello{"hello"};
        send(args.conn_fd, hello.c_str(), hello.size(), 0);

        char recv_buffer[BUFSIZE] = {0};
        char send_buffer[BUFSIZE] = {0};

        while (true) {
            auto length = recv(args.conn_fd, recv_buffer, BUFSIZE, 0);
            if (length < 0) {
                std::cerr << "[server] recv() fails, errno is " << errno << std::endl;
            }

            memset(send_buffer, 0, BUFSIZE);

            args._mtx->lock(); // 互斥锁，确保同一时间只有一个线程访问和处理数据
            // 判断收到的包的类型
            switch (recv_buffer[0]) {
                case DISCONNECT:
                    args._client_list->erase(args.conn_fd);
                    std::cout << "[server] client#" << args.conn_fd << "disconnected\n";
                    break;
                case GET_TIME:
                    time_t t;
                    time(&t);
                    std::cout << "[server] client#" << args.conn_fd << "wants to get time. The time is " << ctime(&t)
                              << std::endl;
                    send_buffer[0] = GET_TIME; // 设置包头
                    sprintf(send_buffer + 1, "%s", ctime(&t));

                    if (send(args.conn_fd, send_buffer, strlen(send_buffer), 0) < 0) {
                        std::cerr << "[server] send fails, errno is " << errno << std::endl;
                    }
                    break;
                case GET_NAME:
                    send_buffer[0] = GET_NAME;
                    gethostname(send_buffer + 1, sizeof(send_buffer) - sizeof(char));
                    std::cout << "[server] client#" << args.conn_fd << "wants to get servername. The name is "
                              << send_buffer + 1
                              << std::endl;
                    if (send(args.conn_fd, send_buffer, strlen(send_buffer), 0) < 0) {
                        std::cerr << "[server] send fails, errno is " << errno << std::endl;
                    }
                    break;
                case GET_ACTIVE_LIST:
                    std::cout << "[server] client#" << args.conn_fd << "wants to get active list\n";
                    send_buffer[0] = GET_ACTIVE_LIST;

                    for (const auto &client: *args._client_list) {
                        fd_t fd = client.first;
                        inet_t ip = client.second.first;
                        port_t port = client.second.second;
                        std::string say =
                                "[server] client#" + std::to_string(fd) + "@" + ip + ":" + std::to_string(port) + "\n";
                        memcpy(send_buffer + strlen(send_buffer), say.c_str(), say.size());
                    }

                    if (send(args.conn_fd, send_buffer, strlen(send_buffer), 0) < 0) {
                        std::cerr << "[server] send fails, errno is " << errno << std::endl;
                    }
                    break;
                case SEND_MSG:
                    std::string data = std::string(recv_buffer + 1);
                    std::string ip = data.substr(0, data.find(':'));

                    data = data.substr(data.find(':') + 1);
                    port_t port = stoi(data.substr(0, data.find(':')));
                    data = data.substr(data.find(':') + 2);

                    std::cout << "[server] client#" << args.conn_fd << " send packet to " << ip << ":" << port
                              << std::endl;

                    fd_t target = -1; // 目标客户端
                    for (const auto &client: *args._client_list) {
                        if (client.second.first == ip && client.second.second == port) {
                            target = client.first;
                            break;
                        }
                    }

                    send_buffer[0] = SEND_MSG;
                    if (target == -1) {
                        sprintf(send_buffer + 1, "[server] no such client");
                    } else {
                        sprintf(send_buffer + 1, "[server] forward success");
                    }

                    char repost[BUFSIZE] = {0};
                    repost[0] = FORWORD;
                    memcpy(repost + 1, data.c_str(), data.size());

                    // 向目标客户端转发
                    if (send(target, repost, strlen(repost), 0) < 0) {
                        std::cerr << "[server] send (repost) fails, errno is" << errno << std::endl;
                    }
                    // 向源客户端回复消息
                    if (send(args.conn_fd, send_buffer, strlen(send_buffer), 0) < 0) {
                        std::cerr << "[server] send (reply) fails, errno is" << errno << std::endl;
                    }
                    break;
            }
            memset(recv_buffer, 0, BUFSIZE);
            args._mtx->unlock();
        }
    }
};

int main() {
    Server server;
    server.run();
    return 0;
}
