
#include "main_srv.h"
#include "consts.h"
#include "dispatcher.h"

int Dispatcher::time_since_start_() {
    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start_time_);
    return 1000 * time_span.count();  // 1000 => shows millisec; 1000000 => shows microsec
}

void Dispatcher::run(string port) {

    // cpu_set_t mask;
    // CPU_ZERO(&mask);
    // CPU_SET(CORE_DISPATCHER_RUNS_ON, &mask);
    // if ( sched_setaffinity(0, sizeof(mask), &mask) > 0) {
    //     cout << "set affinity had an error" << endl;
    // }

    start_time_ = std::chrono::high_resolution_clock::now();

    cout << "dispatcher main process: " << getpid() << endl;

    // start dummySrv
    MainServerImp server;

    std::thread t1(&MainServerImp::Run, &server, port);

    t1.join();
}


int main(int argc, char *argv[]) {
    
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " port\n";
        return 1;
    }

    string port = argv[1];

     // create dispatcher instance
    // hardcoding id for now
    Dispatcher d = Dispatcher(0);

    // run it
    d.run(port);

}   
   
