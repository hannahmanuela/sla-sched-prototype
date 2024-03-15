#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cstring>
#include <numeric>
#include <unistd.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <sys/syscall.h>
#include <sys/resource.h> 
#include <chrono>
#include <ctime>
#include <ratio>
#include <thread>
#include <mutex> 
using namespace std;
using namespace std::chrono;

// sudo setcap cap_sys_nice=ep lnx_test

static clock_t lastCPU, lastSysCPU, lastUserCPU;
static int numProcessors;
high_resolution_clock::time_point global_start;
mutex thread_mtx;

#define NUMITERS 500 // 100
#define NUMTHREADS 25
#define MATSZ 70 // 350
#define THREAD_WAIT_TIME_MICROSEC 500
#define THREAD_WAIT_TIME_MILLISEC 0 // 75
#define UTIL_CHECK_SLEEP_TIME_MILLISEC 5

double timeDiff() {
    duration<double> time_span = duration_cast<duration<double>>(high_resolution_clock::now() - global_start);
    return 1000 * time_span.count();  // 1000 => shows millisec; 1000000 => shows microsec
}

std::vector<size_t> get_cpu_times() {
    std::ifstream proc_stat("/proc/stat");
    // ofstream file;
    // file.open("../proc_files.txt", ios::app);
    // file << proc_stat.rdbuf() << endl << endl;
    // file.close();
    proc_stat.ignore(5, ' '); // Skip the 'cpu' prefix.
    std::vector<size_t> times;
    for (size_t time; proc_stat >> time; times.push_back(time));
    return times;
}

bool get_cpu_times(size_t &idle_time, size_t &total_time) {
    const std::vector<size_t> cpu_times = get_cpu_times();
    if (cpu_times.size() < 4)
        return false;
    idle_time = cpu_times[3];
    total_time = std::accumulate(cpu_times.begin(), cpu_times.end(), 0);
    return true;
}

void readCPU() {

    cpu_set_t  mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask);
    int result = sched_setaffinity(0, sizeof(mask), &mask);

    if ( result > 0) {
        cout << "set affinity had an error" << endl;
    }

    size_t previous_idle_time=0, previous_total_time=0;
    int i = 0;
    for (size_t idle_time, total_time; get_cpu_times(idle_time, total_time); std::this_thread::sleep_for(chrono::milliseconds(UTIL_CHECK_SLEEP_TIME_MILLISEC))) {
        if(i > NUMITERS) {
            return;
        }
        const float idle_time_delta = idle_time - previous_idle_time;
        const float total_time_delta = total_time - previous_total_time;
        const float utilization = 100.0 * (1.0 - idle_time_delta / total_time_delta);

        ofstream file;
        file.open("../polling_info.txt", ios::app);
        file << timeDiff() << ", " << utilization << endl;
        file.close();

        previous_idle_time = idle_time;
        previous_total_time = total_time;
        i++;
    }

}

// The function we want to execute on each thread.
void comp_intense(int* mat1, int* mat2, int thread_num, int matsz) {

    // set affinity to force thread to run on core 0
    cpu_set_t  mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask);
    if ( sched_setaffinity(0, sizeof(mask), &mask) > 0) {
        cout << "set affinity had an error" << endl;
    }

    ofstream file1;
    file1.open("../worker_out.txt", ios::app);
    file1 << timeDiff() << ", " << thread_num << ", 1" << endl;
    file1.close();

    pid_t tid;
    tid = syscall(SYS_gettid);
    if (thread_num == 12) {
        if (setpriority(PRIO_PROCESS, tid, -2) == -1) {
            cout << "setprio failed: " << strerror(errno) << endl;
        }
    }
    int ret = getpriority(PRIO_PROCESS, tid);
    if(ret == -1) {
        cout << "getprio failed" << strerror(errno) << endl;
    }
    
    cout << thread_num << " is running on " << sched_getcpu() << ", with priority " << ret << endl;

    ofstream file2;
    file2.open("../worker_out.txt", ios::app);
    file2 << timeDiff() << ", " << thread_num << ", 2" << endl;
    file2.close();

    high_resolution_clock::time_point beg = high_resolution_clock::now();
 
    // ============================
    // multiply the matrices
    // ============================
    int rslt[matsz][matsz];
    for (int i = 0; i < matsz; i++) {
        for (int j = 0; j < matsz; j++) {
            rslt[i][j] = 0;
            for (int k = 0; k < matsz; k++) {
                int offset1 = i * matsz + k; //mat1[i][k]
                int offset2 = k * matsz + j; //mat2[k][j]
                rslt[i][j] += mat1[offset1] * mat2[offset2];
                // rslt[i][j] += offset1 * offset2;
            }
        }
    }

    high_resolution_clock::time_point end = high_resolution_clock::now();

    duration<double> time_span = duration_cast<duration<double>>(end - beg);

    ofstream file;
    file.open("../worker_out.txt", ios::app);
    file << timeDiff() << ", " <<  thread_num << ", " 
        << 1000 * time_span.count() << ", -1" << endl; // 1000 => shows millisec; 1000000 => shows microsec
    file.close();
}

void emptyFiles() {

    // empty files we write to
    ofstream polling_file;
    polling_file.open("../polling_info.txt");
    polling_file << "";
    polling_file.close();

    ofstream worker_file;
    worker_file.open("../worker_out.txt");
    worker_file << "";
    worker_file.close();

    ofstream proc_file;
    proc_file.open("../proc_files.txt");
    proc_file << "";
    proc_file.close();

}

int main() {

    cout << "pid: " << getpid() << endl;

    emptyFiles();

    // setup
    srand(time(NULL));


    // ============================
    // generate the matrices
    // ============================
    int matsz = MATSZ;
    int *mat1 = (int *)malloc(matsz * matsz * sizeof(int));
    for (int r = 0; r < matsz; r++) {
        for (int c = 0; c < matsz; c++) {
            int offset = r * matsz + c;
            mat1[offset] = rand();
        }
    }

    int *mat2 = (int *)malloc(matsz * matsz * sizeof(int));
    for (int r = 0; r < matsz; r++) {
        for (int c = 0; c < matsz; c++) {
            int offset = r * matsz + c;
            mat2[offset] = rand();
        }
    }

    global_start = high_resolution_clock::now();
    

    // start cpu reading, give a second to generate a baseline
    thread measure(readCPU);
    sleep(1);

    std::vector<thread> threads;

    // measure without contention
    thread t(comp_intense, mat1, mat2, -1, MATSZ);
    threads.push_back(std::move(t));
    sleep(1);
    
    // Constructs the new thread and runs it. Does not block execution.
    for(int i=0; i<NUMTHREADS; i++) {
        thread t(comp_intense, mat1, mat2, i, MATSZ);
        threads.push_back(std::move(t));
        // write start time to file
        ofstream file;
        file.open("../worker_out.txt", ios::app);
        file << timeDiff() << ", " << i << ", 0" << endl;
        file.close();
        // potentiall wait
        std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_WAIT_TIME_MILLISEC));
        std::this_thread::sleep_for(std::chrono::microseconds(THREAD_WAIT_TIME_MICROSEC));
    }

    // Makes the main thread wait for the new thread to finish execution, therefore blocks its own execution.
    for(auto& t : threads){ 
        t.join();
    }
    measure.join();
}

