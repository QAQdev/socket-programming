//
// Created by miboyu on 22-11-26.
//
#include "util.hh"

class pakage{

};
class MsgQueue{
private:
    std::mutex _block_mutex;
    std::list<pakage> msg_list;
    std::condition_variable cv;
public:
    MsgQueue() = default;
    ~MsgQueue() = default;
    void add() {

    }
    void get() {

    }

};