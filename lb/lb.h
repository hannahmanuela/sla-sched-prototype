#include <thread>
#include <grpc/grpc.h>
#include <grpcpp/server_builder.h>

#include "machine.h"
#include "consts.h"
#include "main_clnt.h"
#include "proc_type.h"

using namespace std;

#ifndef LB_H
#define LB_H


class LB {

    public:
        LB() {};

        void init(int argc, char *argv[]);
        void run();

    private:

        MainClient* placeRPCCall(ProcType type);

        vector<MainClient*> dispatchers_;
        map<ProcType, ProcTypeProfile> types_;

};

#endif  // LB_H