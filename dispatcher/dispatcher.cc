#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>

#include "main_srv.h"
#include "website_ex_srv.h"
#include "consts.h"
#include "dispatcher.h"



// --------------------------------
// CODE THAT RUNS UTIL CHECKS
// --------------------------------

long long get_curr_time_ms() {
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    auto epoch = now_ms.time_since_epoch();
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();

    return milliseconds;
}

struct CPUStats {
    long long user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;

    long long get_total_time() const {
        return user + nice + system + idle + iowait + irq + softirq + steal + guest + guest_nice;
    }

    long long get_idle_time() const {
        return idle + iowait;
    }
};

CPUStats read_stats(const std::string& cpu_line) {
    CPUStats stats;
    std::istringstream iss(cpu_line);
    std::string cpu;
    iss >> cpu;  // Skip the "cpu" or "cpu0", "cpu1", etc.
    iss >> stats.user >> stats.nice >> stats.system >> stats.idle >> stats.iowait 
        >> stats.irq >> stats.softirq >> stats.steal >> stats.guest >> stats.guest_nice;
    return stats;
}

void get_stats(std::vector<CPUStats>& stats) {
    std::ifstream file("/proc/stat");
    std::string line;
    while (std::getline(file, line)) {
        if (line.compare(0, 3, "cpu") == 0) {
            stats.push_back(read_stats(line));
        }
    }
}

void dump_stats(const std::vector<double>& usage, const std::string& filename) {
    long long rn = get_curr_time_ms();
    std::ofstream file(filename, std::ios_base::app);
    file << rn << " -- ";
    for (size_t i = 0; i < (std::thread::hardware_concurrency() - 2) / 2; ++i) {
        file << i << ": " << usage[i] << ", ";
    }
    file << std::endl;
}

void calculate_and_dump_usage(const std::string& filename) {
    std::vector<CPUStats> prev_stats;
    std::vector<CPUStats> curr_stats;

    get_stats(prev_stats);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    get_stats(curr_stats);

    std::vector<double> usage(prev_stats.size());

    for (size_t i = 0; i < prev_stats.size(); ++i) {
        long long prev_idle = prev_stats[i].get_idle_time();
        long long curr_idle = curr_stats[i].get_idle_time();
        long long prev_total = prev_stats[i].get_total_time();
        long long curr_total = curr_stats[i].get_total_time();

        long long total_diff = curr_total - prev_total;
        long long idle_diff = curr_idle - prev_idle;

        usage[i] = 100.0 * (1.0 - (double)idle_diff / total_diff);
    }

    dump_stats(usage, filename);
}

void run_util_dumps() {
    const std::string filename = "../util.txt";
    // truncate the file to clear any previous contents
    std::ofstream file(filename, std::ios::trunc);

    while (true) {
        // func sleeps for 10 ms and looks for the diff, logs it
        calculate_and_dump_usage(filename);
    }

}




// --------------------------------
// DISPATCHER CODE
// --------------------------------


int Dispatcher::time_since_start_() {
    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start_time_);
    return 1000 * time_span.count();  // 1000 => shows millisec; 1000000 => shows microsec
}

void Dispatcher::run() {

    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(CORE_DISPATCHER_RUNS_ON, &mask);
    if ( sched_setaffinity(0, sizeof(mask), &mask) > 0) {
        cout << "set affinity had an error" << endl;
    }


    cout << "dispatcher main process: " << getpid() << endl;

    Queue* q = new Queue();

    // start mainSrv
    MainServerImp server = MainServerImp(q);
    WebsiteServerImp web_server = WebsiteServerImp(q);

    std::thread t1(&MainServerImp::Run, &server, DISPATCHER_MAIN_PORT);
    std::thread t2(&WebsiteServerImp::Run, &web_server, DISPATCHER_WEBSITE_PORT);

    std::thread t3(run_util_dumps);

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
   
