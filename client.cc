#include "util.hh"
std::mutex mtx;
std::condition_variable cr;
std::list<Packet> msg_lst;

class Client{
private:
    sockaddr_in _server_address{};
    pthread_t connected_thread{};
    fd_t _socket_fd = -1; // socket file descriptor
    __attribute__((unused)) static int getChoice() {
        //展示菜单
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
        /*
         * 子线程，用于处理收到的数据包
         * 该线程不断循环接收数据包，并加入消息队列。如果数据包类型为FORWARD，还需打印数据包内容
         * 更新消息队列后，唤醒因消息队列为空而进入等待状态的线程
         */
        int fileDescriptor = *(int *)input; //接收数据包的文件描述符
        char buffer[BUFSIZE];
        recv(fileDescriptor, buffer, BUFSIZE, 0); //接收建立连接后服务器发送的确认信息
        while(true) {
            memset(buffer, 0, BUFSIZE);
            auto len = recv(fileDescriptor, buffer, BUFSIZE, 0); //循环接收服务器的应答包
            if (len <= 0) continue;
            std::unique_lock<std::mutex> lck(mtx); //为消息队列加锁
            Packet t{};
            if (buffer[0] == FORWARD) {//转发
                std::cout << "你收到了消息: " << buffer + 1 << std::endl; //打印转发消息
            }
            t.type = static_cast<unsigned char >(buffer[0]); //设置数据包类型
            memcpy(t.data, buffer + 1, len - 1); //设置数据包内容
            msg_lst.push_back(t); //加入消息队列
            cr.notify_all(); //唤醒等待的主线程
        }
    }

    static void printInfo(const std::string& info) {
        // 打印消息
        std::cout << info << std::endl;
    }

    static void printError(const std::string& err) {
        //打印错误信息
        std::cerr << err << std::endl;
    }

    bool isConnectionExists() const {
        //判断当前是否已建立连接
        return _socket_fd != -1;
    }

public:
    Client() = default;
    ~Client() {
        /*
         * 析构时，如果已经建立连接，则关闭
         */
        if (isConnectionExists()) {
            close(_socket_fd);
        }
    }
    [[noreturn]] void run() {
        while (true) {
            int choice = getChoice(); //循环获取用户选择
            if (choice != 1 && choice != 7 && not isConnectionExists()) {
                //除建立连接、退出程序外的选择需要先建立连接
                printError("请先建立连接.");
                continue;
            }
            switch (choice) {
                case 1:{ // 建立连接
                    if (isConnectionExists()) {
                        printInfo("您已经建立了连接");
                        continue;
                    }
                    _socket_fd = socket(AF_INET, SOCK_STREAM, 0);
                    //输入IP和端口
                    std::string ip;
                    int port;
                    printInfo("请输入服务器IP");
                    std::cin >> ip;
                    printInfo("请输入端口");
                    std::cin >> port;
                    //配置连接参数
                    _server_address.sin_family = AF_INET;
                    _server_address.sin_port = htons(port);
                    _server_address.sin_addr.s_addr = inet_addr(ip.c_str());

                    if (connect(_socket_fd, (struct sockaddr*)&_server_address, sizeof(struct sockaddr)) == -1) {
                        //建立连接
                        printError("建立连接失败");
                        close(_socket_fd);
                        _socket_fd = -1;
                        break;
                    }
                    pthread_create(&connected_thread, nullptr, thread_handler, &_socket_fd); //创建接收消息的线程
                    break;
                }
                case 2: { //断开连接
                    char fin = static_cast<char >(DISCONNECT);
                    if (send(_socket_fd, &fin, sizeof fin, 0) <= 0) {
                        //发送结束连接的消息
                        printError("cannot send final signal");
                        break;
                    }
                    //关闭连接
                    close(_socket_fd);
                    _socket_fd = -1;
                    printInfo("连接已断开");
                    break;
                }
                case 3: { //get time
                    char get_time = static_cast<char >(PacketType::GET_TIME);
                    send(_socket_fd, &get_time, sizeof get_time, 0);
                    //发送请求时间的数据包
                    std::unique_lock<std::mutex> lck(mtx);
                    while (msg_lst.empty()) {
                        //如果消息队列为空，说明还未收到回复，则进入等待队列
                        cr.wait(lck);
                    }
                    auto tmp = msg_lst.front(); //收到时间数据包
                    std::cout << "获取时间：" << tmp.data << std::endl; //打印
                    msg_lst.pop_front();
                    break;
                }
                case 4: {
                    char get_name = static_cast<char >(PacketType::GET_NAME);
                    send(_socket_fd, &get_name, sizeof get_name, 0);
                    //发送请求名称的数据包
                    std::unique_lock<std::mutex> lck(mtx);
                    while (msg_lst.empty()) {
                        //如果消息队列为空，说明还未收到回复，则进入等待队列
                        cr.wait(lck);
                    }
                    auto tmp = msg_lst.front();
                    std::cout << "获取服务器名称：" << tmp.data << std::endl;
                    msg_lst.pop_front();
                    break;
                }
                case 5: {
                    char get_list = static_cast<char >(PacketType::GET_ACTIVE_LIST);
                    send(_socket_fd, &get_list, sizeof get_list, 0);
                    //发送请求客户端列表的数据包
                    std::unique_lock<std::mutex> lck(mtx);
                    while (msg_lst.empty()) {
                        //如果消息队列为空，说明还未收到回复，则进入等待队列
                        cr.wait(lck);
                    }
                    auto tmp = msg_lst.front();
                    std::cout << "获取用户列表：" << tmp.data << std::endl;
                    msg_lst.pop_front();
                    break;
                }
                case 6: {
                    char buffer[BUFSIZE]{0};
                    //获取要发送的IP和端口号
                    std::string ip;
                    int port;
                    printInfo("请输入要发送的IP");
                    std::cin >> ip;
                    printInfo("请输入要发送的端口");
                    std::cin >> port;
                    buffer[0] = SEND_MSG;
                    printInfo("请输入要发送的信息，#结束");
                    sprintf(buffer + 1, "%s:%d:\n", ip.c_str(), port);
                    char c = '\0';
                    while(c != '#') {
                        //获取输入
                        std::cin >> c;
                        sprintf(buffer + strlen(buffer), "%c", c);
                    }
                    //接收转发消息的回复(转发信息发送后，服务器也会向发送消息的客户端发送应答包)
                    send(_socket_fd, &buffer, sizeof buffer, 0);
                    std::unique_lock<std::mutex> lck(mtx);
                    while (msg_lst.empty()) {
                        cr.wait(lck);
                    }
                    auto tmp = msg_lst.front();
                    std::cout << "发送信息：" << tmp.data << std::endl;
                    msg_lst.pop_front();
                    break;
                }
                case 7: {
                    //退出程序
                    if (isConnectionExists()) {
                        //如果连接存在，先断开
                        char fin = static_cast<char >(DISCONNECT);
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
    client.run();
}
