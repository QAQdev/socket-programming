#include "util.hh"

class Server {
private:
    std::string _server_name{"oneko"}; // 服务端名称
    fd_t _sock_fd; // 套接字文件描述符
    sockaddr_in _sock_addr_in; // 套接字地址
    static constexpr int _max_connection = 20; // 最大连接数
    port_t _default_port = 5708; // 学号后 4 位作为端口号
    std::map<fd_t, client_t> _client_list;

public:

    /*
     * getter & setter
     */

    const std::string &server_name() { return _server_name; }

    int &sock_fd() { return _sock_fd; }

    sockaddr_in &sock_addr_in() { return _sock_addr_in; }

    static constexpr int max_connection() { return _max_connection; }

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
        std::cout << "server " + _server_name + " is running\n";

        while (true) {
            sockaddr_in client;
            int client_addr_len = sizeof(client);
            // 阻塞式等待连接请求
            int connection_fd = accept(_sock_fd, (sockaddr *) &client, (socklen_t *) &client_addr_len);

            // 将客户端信息加入列表
            _client_list.insert(std::make_pair(
                    connection_fd, client_t(inet_ntoa(client.sin_addr), ntohs(client.sin_port));
            ));
            std::cout << inet_ntoa(client.sin_addr) << ":" << ntohs(client.sin_port) << " connected\n";

            // BIO 模式，为每个客户端建立一个线程监听请求
            pthread_t client_thread;
            pthread_create(&client_thread, nullptr, thread)
        }
    }
};
