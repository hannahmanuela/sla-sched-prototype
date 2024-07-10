#include <vector>
#include <string>
#include <unistd.h>
#include <sys/socket.h>

#include <grpc/grpc.h>

#ifndef MACHINE_H
#define MACHINE_H

// the LBs representation of a machine
class Machine {

    public:
        int mid;
        std::shared_ptr<grpc::Channel> channel;
        int mem_free; // in what unit?

        Machine(int id, std::shared_ptr<grpc::Channel> chan) {
            mid = id;
            channel = chan;
        }

    private:

};

#endif  // MACHINE_H