#include <thread>
#include <iostream>
#include <climits> 
#include <stdlib.h> 
#include <chrono>
#include <fstream>
#include <sstream>

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

using namespace std;

#include "lb.h"

string load_file = "../load.txt";
string latency_file = "../latency.txt";

int total_load = 0;
std::mutex load_lock;


void writeToOutFile(string filename, string to_write) {
    ofstream myfile;
    myfile.open(filename, std::ios_base::app);
    myfile << to_write;
    myfile.close();
}

long long get_curr_time_ms() {
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    auto epoch = now_ms.time_since_epoch();
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();

    return milliseconds;
}


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

void LB::runProc(WebsiteClient* to_use, ProcType type, string type_str) {

    load_lock.lock();
    total_load += types_.at(type).compute_max;
    load_lock.unlock();

    long long time_started = get_curr_time_ms();
    auto start_time = std::chrono::high_resolution_clock::now();

    RetVal reply = to_use->MakeCall(types_.at(type).mem->avg + types_.at(type).mem->std_dev, 
        types_.at(type).compute_max, types_.at(type).deadline, type_str);
    
    std::chrono::duration<double> since_start = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start_time);
    int ms_since_start = 1000 * since_start.count();
    
    std::ostringstream to_write;
    to_write << time_started << " - latency: time passed: (inside: " << reply.timepassed() << ", outside: " 
        << ms_since_start << "), deadline: " << types_.at(type).deadline  << endl;
    writeToOutFile(latency_file, to_write.str());

    load_lock.lock();
    total_load -= types_.at(type).compute_max;
    load_lock.unlock();

}

void LB::runBench() {

    vector<thread> threads;

    // truncate files to clear any previous contents
    std::ofstream f1(load_file, std::ios::trunc);
    f1.close();
    std::ofstream f2(latency_file, std::ios::trunc);
    f2.close();
    
    for (int i = 0; i < NUM_REPS; i++) {

        int rand_perc = rand() % 100;

        ProcType type = DATA_PROCESS_BG;
        string type_str = "bg";

        if (rand_perc < PERCENT_STATIC) {
            type = STATIC_PAGE_GET;
            type_str = "static";
        } else if (rand_perc < PERCENT_STATIC + PERCENT_DYNAMIC) {
            type = DYNAMIC_PAGE_GET;
            type_str = "dynamic";
        } else if (rand_perc < PERCENT_STATIC + PERCENT_DYNAMIC + PERCENT_FG) {
            type = DATA_PROCESS_FG;
            type_str = "fg";
        }

        WebsiteClient* to_use = pickDispatcher(type);
        if (!to_use) {
            cout << "no good dispatcher found -- rejecting" << endl;
            continue;
        }

        
        // what I actually want is load in the system...
        load_lock.lock();
        std::ostringstream to_write;
        to_write << get_curr_time_ms() << ", load: " << total_load << endl;
        writeToOutFile(load_file, to_write.str());
        load_lock.unlock();

        thread t(&LB::runProc, this, to_use, type, type_str);
        threads.push_back(move(t));

        // say we want to gen 27 ms of work per ms (util of 50%) => 27000 ms of work per s
        // (weighted) avg ms of work gen by proc is ~30
        // 27000 / 30 = 900
        // so if we gen 900 procs per sec, we will average out correctly

        // this_thread::sleep_for(chrono::milliseconds(1));
    
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
    lb->runBench();

    return 0;
}