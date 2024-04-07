#include <vector>
#include <string>
#include <unistd.h>
#include <sys/socket.h>

#include "message.h"
#include "consts.h"

#ifndef MACHINE_H
#define MACHINE_H

// the LBs representation of a machine
class Machine {

    public:
        int mid;
        int conn_fd;

        Machine(int id, int fd) {
            mid = id;
            conn_fd = fd;
        }

        void send_message(Message to_send) {
            char buffer[BUF_SZ];
            to_send.to_bytes(buffer);
            ssize_t n = send(conn_fd, buffer, sizeof(buffer), 0);
            if (n < 0) {
                perror("ERROR sending to socket");
            }
            cout << "sent " << n << " bytes via " << conn_fd << endl;
        }


    private:

};

#endif  // MACHINE_H