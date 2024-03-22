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
#include <sys/wait.h> 
#include <fcntl.h>
#include <linux/sched.h>
#include <sched.h>
#include <chrono>
#include <ctime>
#include <ratio>
#include <thread>
using namespace std;
using namespace std::chrono;

// sudo setcap cap_sys_nice=ep lnx_test
// sudo setcap cap_sys_admin+ep lnx_test
// don't forget to chown the cgroups

static clock_t lastCPU, lastSysCPU, lastUserCPU;
static int numProcessors;
high_resolution_clock::time_point global_start;

#define NUMITERS 500 // 100
#define NUM_5_MS_SLA 0
#define NUM_50_MS_SLA 0 
#define NUM_500_MS_SLA 1
#define UTIL_CHECK_SLEEP_TIME_MILLISEC 5
#define STACK_SIZE 1048576 // 1024 * 1024, like in man page example

double timeDiff() {
    duration<double> time_span = duration_cast<duration<double>>(high_resolution_clock::now() - global_start);
    return 1000 * time_span.count();  // 1000 => shows millisec; 1000000 => shows microsec
}

std::vector<size_t> get_cpu_times() {
    std::ifstream proc_stat("/proc/stat");
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
void comp_intense() {
    sleep(60);
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

    emptyFiles();

    int fd_500 = open("/sys/fs/cgroup/three-digit-ms/cgroup.procs", O_RDWR);
    if(fd_500 == -1) {
        cout << "open failed: " << strerror(errno) << endl;
    }
    int fd_50 = open("/sys/fs/cgroup/two-digit-ms/cgroup.procs", O_RDWR);
    if(fd_50 == -1) {
        cout << "open failed: " << strerror(errno) << endl;
    }
    int fd_5 = open("/sys/fs/cgroup/single-digit-ms/cgroup.procs", O_RDWR);
    if(fd_5 == -1) {
        cout << "open failed: " << strerror(errno) << endl;
    }

    cout << "fd_500: " << fd_500 << endl;
    cout << "fd_50: " << fd_50 << endl;
    cout << "fd_5: " << fd_5 << endl;

    global_start = high_resolution_clock::now();
    
    // start cpu reading, give a second to generate a baseline
    // thread measure(readCPU);
    // sleep(1);

    std::vector<pid_t> procs;
    
    for(int i=0; i<(NUM_5_MS_SLA + NUM_50_MS_SLA + NUM_500_MS_SLA); i++) {

        __u64 stack = (__u64)malloc(STACK_SIZE);

        int fd_to_use = -1;
         if (i < NUM_500_MS_SLA) {
            cout << "giving fd 500 " << endl;
            fd_to_use = fd_500;
        } else if (i < (NUM_500_MS_SLA + NUM_50_MS_SLA)) {
            cout << "giving fd 50 " << endl;
            fd_to_use = fd_50;
        } else {
            cout << "giving fd 5" << endl;
            fd_to_use = fd_5;
        }

        struct clone_args args = {
            .flags = CLONE_VM | CLONE_INTO_CGROUP,
            .exit_signal = SIGCHLD,
            .stack = stack,
            .stack_size = STACK_SIZE,
            .cgroup = fd_to_use,
        }; 

        
        int c_pid = syscall(SYS_clone3, &args, sizeof(struct clone_args));
  
        if (c_pid == -1) { 
            cout << "clone3 failed: " << strerror(errno) << endl;
            perror("clone3"); 
            exit(EXIT_FAILURE); 
        } else if (c_pid > 0) { 
            // if parent, add pid to list
            procs.push_back(c_pid);
            cout << "child pid: " << c_pid << endl;
            // write start time to file
            // ofstream file;
            // file.open("../worker_out.txt", ios::app);
            // file << timeDiff() << ", " << i << ", 0" << endl;
            // file.close();
        } else { 
            // if child, execute intense compute
            comp_intense();
            // break from loop
            break;
        }   
    }

    for(auto& c_pid : procs){ 
        waitpid(c_pid, NULL, 0);
    }
    // measure.join();
}

