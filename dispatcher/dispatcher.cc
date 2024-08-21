
#include "main_srv.h"
#include "website_ex_srv.h"
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


    cout << "dispatcher main process: " << getpid() << endl;

    Queue* q = new Queue();

    // start mainSrv
    MainServerImp server;
    WebsiteServerImp web_server = WebsiteServerImp(q);

    std::thread t1(&MainServerImp::Run, &server, DISPATCHER_MAIN_PORT, q);
    std::thread t2(&WebsiteServerImp::Run, &web_server, DISPATCHER_WEBSITE_PORT);

    t1.join();
    t2.join();
}


int main() {

     // create dispatcher instance
    // hardcoding id for now
    Dispatcher d = Dispatcher(0);

    // run it
    d.run();

}   
   
