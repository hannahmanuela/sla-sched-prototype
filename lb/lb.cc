#include <thread>
#include <iostream>
#include <climits> 

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

using namespace std;

#include "lb.h"


MainClient* LB::placeRPCCall(ProcType type) {

    ProcTypeProfile profile = types_.at(type);

    // pick a dispatcher
    vector<MainClient*> contender_dispatchers;
    MainClient* disp_to_use;

    int min_val = INT_MAX;
    for (auto disp : dispatchers_) {
        PlacementReply reply = disp->OkToPlace(profile.mem->avg + profile.mem->std_dev, profile.compute_max);
        if (reply.oktoplace() && reply.ratio() < min_val)  {
            min_val = reply.ratio();
            disp_to_use = disp;
        }
    }

    return disp_to_use;
}

void LB::run() {

    // TODO: remove this test
    for (auto disp : dispatchers_) {
        PlacementReply reply = disp->OkToPlace(10.0, 10.0);
        cout << "got reply from dispatcher, ok: " << reply.oktoplace() << ", ratio: " << reply.ratio() << endl;
    }

    // gen load
    // place each proc

}


void LB::init(int argc, char *argv[]) {

    // read in all the port numbers
    vector<string> port_numbers;
    for (int i = 1; i < argc; ++i) {
        string port = argv[i];
        port_numbers.push_back(port);
    }

    // populate the proc profiles - hardcoded for now
    // TODO: this

    // connect to all the dispatchers
    for (string dispatcher_port : port_numbers) {
        string dispatcher_addr = "0.0.0.0:" + dispatcher_port;
        MainClient* clnt = new MainClient(grpc::CreateChannel(dispatcher_addr, grpc::InsecureChannelCredentials()));

        dispatchers_.push_back(clnt);
    }

    cout << "LB started" << endl;

}



int main(int argc, char *argv[]) {
    
    if (argc < 2) {
        cout << "Usage: " << argv[0] << " <port1> <port2> ... <portN>" << endl;
        return 1;
    }
    
    LB* lb = new LB();

    lb->init(argc, argv);
    lb->run();

    return 0;
}