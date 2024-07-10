#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <syscall.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <thread>
#include <fstream>

#include "dummy_srv.h"
#include "consts.h"
#include "dispatcher.h"

int Dispatcher::time_since_start_() {
    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start_time_);
    return 1000 * time_span.count();  // 1000 => shows millisec; 1000000 => shows microsec
}

void Dispatcher::run() {

    // cpu_set_t mask;
    // CPU_ZERO(&mask);
    // CPU_SET(CORE_DISPATCHER_RUNS_ON, &mask);
    // if ( sched_setaffinity(0, sizeof(mask), &mask) > 0) {
    //     cout << "set affinity had an error" << endl;
    // }

    start_time_ = std::chrono::high_resolution_clock::now();

    cout << "dispatcher main process: " << getpid() << endl;

    // start dummySrv
    DummyServerImp server;
    server.Run(2);
}




int main() {    
    // create dispatcher instance
    // hardcoding id for now
    Dispatcher d = Dispatcher(0);

    // run it
    d.run();
}
