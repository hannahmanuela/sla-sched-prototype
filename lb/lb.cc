#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include <iostream>

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

using namespace std;

#include "lb.h"
#include "consts.h"
#include "dummy_clnt.h"


void doTheThing(std::string inp, DummyClient* clnt) {
        std::string reply = clnt->DoStuff(inp); 
        std::cout << "resp received: " << reply << std::endl;  
}

void LB::runDummyEx() {

    DummyClient clnt(grpc::CreateChannel(DISPATCHER_ADDR, grpc::InsecureChannelCredentials()));

    std::vector<std::thread> threads;
    
    for (int i = 0; i < 10; i++) {
        std::string inp = std::to_string(i);
        // doTheThing(inp, &clnt);
        std::thread t(doTheThing, inp, &clnt);
        threads.push_back(std::move(t));
    }

    for (std::thread& t : threads) {
        t.join();
    }

}

void LB::run() {

    // cpu_set_t mask;
    // CPU_ZERO(&mask);
    // CPU_SET(CORE_LB_WEBSITE_RUN_ON, &mask);
    // if ( sched_setaffinity(0, sizeof(mask), &mask) > 0) {
    //     cout << "set affinity had an error" << endl;
    // }

    cout << "LB main process: " << getpid() << endl;

    std::thread client_thread(&LB::runDummyEx, this);

    client_thread.join();
}




int main() {
    // create lb instance
    LB* lb = new LB();
    // run it
    lb->run();
}