#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <syscall.h>
#include <signal.h>
#include <iostream>
#include <fstream>
using namespace std;

#include "website.h"
#include "proc.h"
#include "consts.h"
#include "message.h"

int Website::connect_to_lb_() {
    int status, lb_conn_fd;
    struct sockaddr_in serv_addr;
    if ((lb_conn_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(LB_CLIENT_LISTEN_PORT);
 
    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }
 
    if ((status = connect(lb_conn_fd, (struct sockaddr*)&serv_addr,
                   sizeof(serv_addr))) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    cout << "connected to lb" << endl;

    return lb_conn_fd;
}

void Website::gen_load(int lb_conn_fd) {

    for (int i=0; i<(NUM_STATIC_PROCS_GEN + NUM_DYNAMIC_PROCS_GEN + NUM_DATA_FG_PROCS_GEN); i++) {

        const char* executable;
        int sla;
        ProcType type;
        if (i < NUM_STATIC_PROCS_GEN) {
            executable = "/home/hannahmanuela/strawman-microbench/build/static_page_get";
            sla = 5;
            type = static_page_get; 
        } else if (i < NUM_STATIC_PROCS_GEN + NUM_DYNAMIC_PROCS_GEN) {
            executable = "/home/hannahmanuela/strawman-microbench/build/dynamic_page_get";
            sla = 50;
            type = dynamic_page_get;
        } else {
            executable = "/home/hannahmanuela/strawman-microbench/build/data_process_fg";
            sla = 500;
            type = data_process_fg;
        }

        // send "proc"
        vector<const char*> command;
        command.push_back(executable);
        Proc* proc = new Proc(sla, command, type);
        Message to_send = Message(-1, false, proc);
        char buffer[BUF_SZ];
        to_send.to_bytes(buffer);

        ssize_t n = send(lb_conn_fd, buffer, sizeof(buffer), 0);
        if (n < 0) {
            perror("ERROR sending to socket");
        }
        cout << "sent " << n << " bytes" << endl;
        // TODO: delete alloced stuff?
    }

    // send exit message
    cout << "sending exit!" << endl;
    Message to_send = Message(5, true, new Proc());
    char buffer[BUF_SZ];
    to_send.to_bytes(buffer);
    ssize_t n = send(lb_conn_fd, buffer, sizeof(buffer), 0);
    if (n < 0) {
        perror("ERROR sending to socket");
    }
    cout << "sent " << n << " bytes" << endl;
}

void Website::run() {

    cout << "website main process: " << getpid() << endl;
    
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(CORE_LB_WEBSITE_RUN_ON, &mask);
    if ( sched_setaffinity(0, sizeof(mask), &mask) > 0) {
        cout << "set affinity had an error" << endl;
    }

    int lb_conn_fd = connect_to_lb_();
    sleep(1);

    gen_load(lb_conn_fd);
    cout << "CLIENT EXITED" << endl;
}

int main() {
    // create website instance
    Website w = Website();
    // run it
    w.run();
}