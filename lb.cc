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
            cout << "client died";
            return;
        }
        Message client_msg = Message();
        client_msg.from_bytes(read_buffer);

        if (client_msg.is_exit) {
            cout << "client closed conn" << endl;
            return;
        }

        cout << "got proc from client: ";
        client_msg.msg_proc->print();

        // TODO: pick a machine to send it to
        // for now just sending to first one
        if (machines_.size() < 1) {
            cout << "oops no machine, dropping for now" << endl;
            continue;
        }
        machine_list_lock_.lock();
        Machine* machine_to_use = machines_[0];
        machine_list_lock_.unlock();

        machine_to_use->send_message(Message(-1, false, client_msg.msg_proc));
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

    client_thread.join();
}




int main() {
    // create lb instance
    LB* lb = new LB();
    // run it
    lb->run();
}