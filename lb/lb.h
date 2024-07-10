#include <mutex>
#include <grpc/grpc.h>
#include <grpcpp/server_builder.h>

#include "machine.h"

#ifndef LB_H
#define LB_H


class LB {

    public:
        LB() {};
        void run();

    private:

        void runDummyEx();

        // std::vector<Machine*> machines_;
        // std::mutex machine_list_lock_;

};

#endif  // LB_H