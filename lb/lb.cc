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


WebsiteClient* LB::pickDispatcher(ProcType type) {

    ProcTypeProfile profile = types_.at(type);

    // pick a dispatcher
    vector<MainClient*> contender_dispatchers;
    WebsiteClient* disp_to_use = nullptr;

    int min_val = INT_MAX;
    for (auto disp : dispatchers_) {
        PlacementReply reply = get<0>(disp)->OkToPlace(profile.mem->avg + profile.mem->std_dev, profile.compute_max, profile.deadline);
        if (reply.oktoplace() && reply.ratio() < min_val)  {
            min_val = reply.ratio();
            disp_to_use = get<1>(disp);
        }
    }

    return disp_to_use;
}

void LB::runProc(WebsiteClient* to_use, string type) {

    RetVal reply = to_use->MakeCall(types_.at(STATIC_PAGE_GET).deadline, types_.at(STATIC_PAGE_GET).compute_max, 
        types_.at(STATIC_PAGE_GET).mem->avg + types_.at(STATIC_PAGE_GET).mem->std_dev, type);
    cout << "sum: " << reply.retval() << ", took rusage: " << reply.rusage() << " and wall-clock time " << reply.timepassed() << endl;

}

void LB::run() {

    vector<thread> threads;

    // gen load, place each proc
    for (int i = 0; i < 2; i++) {
        ProcType type = DYNAMIC_PAGE_GET;
        ProcTypeProfile profile = types_.at(type);
        string type_str = "dynamic";
        WebsiteClient* to_use = pickDispatcher(type);
        if (!to_use) {
            cout << "no good dispatcher found" << endl;
            break;
        }
        thread t(&LB::runProc, this, to_use, type_str);
        threads.push_back(move(t));
        this_thread::sleep_for(chrono::milliseconds(10));
    }

    for (auto& t : threads) {
        t.join();
    }

}


void LB::init() {
    // populate the proc profiles - hardcoded for now

    // TODO: this
    // init_mem(MB), comp_max, dl (ms)
    ProcTypeProfile static_get = ProcTypeProfile(50, 9, 10);
    types_.insert({STATIC_PAGE_GET, static_get});
    
    ProcTypeProfile dynamic_get = ProcTypeProfile(100, 50, 60);
    types_.insert({DYNAMIC_PAGE_GET, dynamic_get});

    ProcTypeProfile fg = ProcTypeProfile(500, 1000, 1200);
    types_.insert({DATA_PROCESS_FG, fg});

    ProcTypeProfile bg = ProcTypeProfile(1000, 2800, 5000);
    types_.insert({DATA_PROCESS_BG, bg});

    // connect to all the dispatchers
    MainClient* main_clnt = new MainClient(grpc::CreateChannel(string("0.0.0.0:") + DISPATCHER_MAIN_PORT, grpc::InsecureChannelCredentials()));
    WebsiteClient* website_clnt = new WebsiteClient(grpc::CreateChannel(string("0.0.0.0:") + DISPATCHER_WEBSITE_PORT, grpc::InsecureChannelCredentials()));

    dispatchers_.push_back({main_clnt, website_clnt});

    cout << "LB started" << endl;

}



int main() {
    
    LB* lb = new LB();

    lb->init();
    lb->run();

    return 0;
}