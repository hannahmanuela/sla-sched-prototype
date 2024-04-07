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

    for (int i=0; i<NUM_PROCS_GEN; i++) {
        // send "proc"
        vector<const char*> command;
        command.push_back("build/static_page_get");

        Proc* proc = new Proc(5, command, static_page_get);
        Message to_send = Message(5, false, proc);
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