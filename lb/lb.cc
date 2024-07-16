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
        cout << "got ret val " << reply.oktoplace() << endl;
        if (reply.oktoplace() && reply.ratio() < min_val)  {
            min_val = reply.ratio();
            disp_to_use = get<1>(disp);
        }
    }

    return disp_to_use;
}

void LB::runProc(ProcTypeProfile profile, WebsiteClient* to_use) {

    to_use->GetProfilePage(profile.mem->avg + profile.mem->std_dev, profile.compute_max, profile.deadline);
}

void LB::run() {

    vector<thread> threads;

    // gen load, place each proc
    for (int i = 0; i < 5; i++) {
        ProcType type = DYNAMIC_PAGE_GET;
        ProcTypeProfile profile = types_.at(type);
        WebsiteClient* to_use = pickDispatcher(type);
        if (to_use == nullptr) {
            cout << "no good dispatcher found" << endl;
            break;
        }
        thread t(&LB::runProc, this, profile, to_use);
        threads.push_back(move(t));
    }

    for (auto& t : threads) {
        t.join();
    }

}


void LB::init() {
    // populate the proc profiles - hardcoded for now
    // TODO: this
    // 100 MB, 200ms, 250ms
    ProcTypeProfile dynamic = ProcTypeProfile(100, 200, 250);
    types_.insert({DYNAMIC_PAGE_GET, dynamic});

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