#include <thread>
#include <grpc/grpc.h>
#include <grpcpp/server_builder.h>

#include "machine.h"
#include "consts.h"
#include "main_clnt.h"
#include "website_ex_clnt.h"
#include "proc_type.h"

using namespace std;

#ifndef LB_H
#define LB_H


class LB {

    public:
        LB() {};

        void init();
        void runBench();
        void runProc(WebsiteClient* to_use, ProcType type, string type_str);

    private:

        WebsiteClient* pickDispatcher(ProcType type);

        vector<tuple<MainClient*, WebsiteClient*>> dispatchers_;
        map<ProcType, ProcTypeProfile> types_;

};

#endif  // LB_H