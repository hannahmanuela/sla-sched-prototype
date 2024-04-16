#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include <iostream>
using namespace std;

#include "lb.h"
#include "message.h"
#include "consts.h"

void LB::listen_and_accept_machines_() {
    // create socket
    int listen_fd, machine_conn_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(LB_MACHINE_LISTEN_PORT);

    if (bind(listen_fd, (struct sockaddr*)&address,
             sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // open socket, listen
    if (listen(listen_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    cout << "listening for machines" << endl;
    // accept connections as they come in
    // TODO: at some point we will need to deal with machines leaving
    while (1) {
        if ((machine_conn_fd= accept(listen_fd, (struct sockaddr*)&address,
                  &addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        cout << "got machine in " << machine_conn_fd << endl;
        machine_list_lock_.lock();
        Machine* m = new Machine(machines_.size(), machine_conn_fd);
        machines_.push_back(m);
        machine_list_lock_.unlock();
    }
    close(listen_fd);
}

void LB::listen_and_accept_clients_() {

    // create socket
    int listen_fd, client_conn_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(LB_CLIENT_LISTEN_PORT);

    if (bind(listen_fd, (struct sockaddr*)&address,
             sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    cout << "listening for clients" << endl;

    // open socket, listen
    if (listen(listen_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    // accept connections as they come in
    std::vector<thread> client_threads;
    while (1) {
        if ((client_conn_fd= accept(listen_fd, (struct sockaddr*)&address,
                  &addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        cout << "got client in " << client_conn_fd << endl;

        std::thread t(&LB::handle_client_conn_, this, client_conn_fd);
        client_threads.push_back(std::move(t));
    }
    for(auto& t : client_threads){ 
        t.join();
    }
    close(listen_fd);
}

void LB::handle_client_conn_(int client_conn_fd) {

    while (1) {
        char read_buffer[BUF_SZ];
        int n = read(client_conn_fd, read_buffer, BUF_SZ);
        if (n < 0) {
            perror("ERROR reading from socket");
        } else if (n == 0) {
            cout << "client died" << endl;
            return;
        }
        MessageType msg_type;
        std::memcpy(&msg_type, read_buffer, sizeof(MessageType));

        if (msg_type == EXIT) {
            cout << "client closed conn" << endl;
            return;
        } else if (msg_type != PROC) {
            cout << "why is client sending not exit or proc msg?" << endl;
            return;
        }
        ProcMessage client_proc_msg = ProcMessage();
        client_proc_msg.from_bytes(read_buffer);

        cout << "got proc from client: ";
        client_proc_msg.msg_proc->print();

        // pick a machine to send it to
        if (machines_.size() < 1) {
            cout << "oops no machine, dropping for now" << endl;
            continue;
        }
        machine_list_lock_.lock();
        int machine_ind = 0;
        if (client_proc_msg.msg_proc->get_sla() < THRESHOLD_SPRAY_SLA) {
            machine_ind = (std::rand() % machines_.size());
        } else {
            // TODO: right now we are IGNORING memory :/ 
            // find minimum number of procs in bucket across all machines that we know about
            int min_procs_in_ind = INT32_MAX;
            for (Machine* m : machines_) {
                tuple<int, int> m_vals = m->curr_hist.get(client_proc_msg.msg_proc->get_sla());
                int sum = get<0>(m_vals) + get<1>(m_vals);
                if (sum < min_procs_in_ind) {
                    min_procs_in_ind = sum;
                }
            }
            cout << "min procs in ind: " << min_procs_in_ind << endl;
            // if there are multiple machines with that value, pick the one with the least number of overall procs
            int min_procs_total = INT32_MAX;
            int index = 0;
            for (Machine* m : machines_) {
                tuple<int, int> m_vals = m->curr_hist.get(client_proc_msg.msg_proc->get_sla());
                int sum = get<0>(m_vals) + get<1>(m_vals);
                if ((sum == min_procs_in_ind) && sum < min_procs_total)  {
                    cout << "using machine " << index << ", because of its small small sum " << sum << endl;
                    machine_ind = index;
                }
                index += 1;
            }
        }
        cout << "machine to use: " << machine_ind << endl;
        Machine* machine_to_use = machines_[machine_ind];

        machine_to_use->send_message(client_proc_msg);
        machine_list_lock_.unlock();
    }
    
}

void wait_on_heartbeat(Machine* m) {

    while (1) {
        char read_buffer[BUF_SZ];
        int n = read(m->conn_fd, read_buffer, BUF_SZ);
        if (n < 0) {
            perror("ERROR reading from socket");
        } else if (n == 0) {
            return;
        }
        MessageType msg_type;
        std::memcpy(&msg_type, read_buffer, sizeof(MessageType));

        if (msg_type != HEARTBEAT_RESPONSE) {
            cout << "response to heartbeat wasn't a heartbeat reponse?" << endl;
            return;
        } else {
            HeartbeatResponseMessage resp = HeartbeatResponseMessage();
            resp.from_bytes(read_buffer);
            m->curr_hist = resp.hist;
        }

    }
}

void LB::heartbeat_machines_() {

    HeartbeatMessage heartbeat = HeartbeatMessage();
    std::vector<std::thread> active_heartbeats;

    while (1) {
        for (Machine* m : get_machines_()) {
            // ping machine
            m->send_message(heartbeat);
            // have a thread wait until time is up or have response; update machine info locally
            std::thread h(wait_on_heartbeat, m);
            active_heartbeats.push_back(std::move(h));
        }
        std::this_thread::sleep_for(chrono::milliseconds(MACHINE_HEARTBEAT_SLEEP_TIME_MILLISEC));
    }

}

void LB::run() {

    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(CORE_LB_WEBSITE_RUN_ON, &mask);
    if ( sched_setaffinity(0, sizeof(mask), &mask) > 0) {
        cout << "set affinity had an error" << endl;
    }

    cout << "LB main process: " << getpid() << endl;

    std::thread machine_thread(&LB::listen_and_accept_machines_, this);
    std::thread client_thread(&LB::listen_and_accept_clients_, this);
    std::thread heartbeat_machines(&LB::heartbeat_machines_, this);

    client_thread.join();
}




int main() {
    // create lb instance
    LB* lb = new LB();
    // run it
    lb->run();
}