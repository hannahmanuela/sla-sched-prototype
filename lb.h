#include <mutex>

#include "machine.h"

#ifndef LB_H
#define LB_H


class LB {

    public:
        LB() {};
        void run();


    private:
        void listen_and_accept_machines_();
        void listen_and_accept_clients_();

        void handle_client_conn_(int conn_fd);
        void heartbeat_machines_();

        std::vector<Machine*> get_machines_() {
            machine_list_lock_.lock();
            std::vector<Machine*> to_ret = machines_;
            machine_list_lock_.unlock();
            return to_ret;
        }

        std::vector<Machine*> machines_;
        std::mutex machine_list_lock_;

};

#endif  // LB_H