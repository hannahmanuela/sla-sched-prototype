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

static clock_t lastCPU, lastSysCPU, lastUserCPU;
static int numProcessors;
high_resolution_clock::time_point global_start;

#define NUMITERS 500 // 100
#define NUM_5_MS_SLA 1
#define NUM_50_MS_SLA 1
#define NUM_500_MS_SLA 1
#define UTIL_CHECK_SLEEP_TIME_MILLISEC 5
#define TO_FACTOR_500 87465353
#define TO_FACTOR_50 87465352
#define TO_FACTOR_5 87465351

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
void comp_intense(int worker_num, int type) {

    cpu_set_t mask;
    long nproc, i;
    if (sched_getaffinity(0, sizeof(cpu_set_t), &mask) == -1) {
        perror("sched_getaffinity");
    }
    nproc = sysconf(_SC_NPROCESSORS_ONLN);
    cout << worker_num << " affinity: ";
    for (i = 0; i < nproc; i++) {
        cout <<  CPU_ISSET(i, &mask) << " ";
    }
   cout << endl;

    ofstream file;
    file.open("../worker_out.txt", ios::app);
    file << timeDiff() << ", " << worker_num << ", " << type << ", 1" << endl;
    ofstream trash_file;
    trash_file.open("../trash.txt", ios::app);

    high_resolution_clock::time_point beg = high_resolution_clock::now();
 
    // ============================
    // do the thing
    // ============================
    int to_factor;
    if (type == 500) {
        to_factor = TO_FACTOR_500;
    } else if (type == 50) {
        to_factor = TO_FACTOR_50;
    } else {
        to_factor = TO_FACTOR_5;
    }

    for (int i = 1; i <= to_factor; i++) 
        if (to_factor % i == 0) 
            trash_file << " " << i << endl; 

    high_resolution_clock::time_point end = high_resolution_clock::now();

    duration<double> time_span = duration_cast<duration<double>>(end - beg);

    file << timeDiff() << ", " <<  worker_num  << ", " << type << ", " 
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

    ofstream trash_file;
    trash_file.open("../trash.txt");
    trash_file << "";
    trash_file.close();

}


int main() {

    cpu_set_t  mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask);
    if ( sched_setaffinity(0, sizeof(mask), &mask) > 0) {
        cout << "set affinity had an error" << endl;
    }

    cpu_set_t mask_2;
    long nproc, i;
    if (sched_getaffinity(0, sizeof(cpu_set_t), &mask_2) == -1) {
        perror("sched_getaffinity");
    }
    nproc = sysconf(_SC_NPROCESSORS_ONLN);
    cout << "parent affinity: ";
    for (i = 0; i < nproc; i++) {
       cout << CPU_ISSET(i, &mask_2) << " ";
    }
    cout << endl;


    emptyFiles();

    int fd_500 = open("/sys/fs/cgroup/three-digit-ms/cgroup.procs", O_RDWR | O_APPEND);
    if(fd_500 == -1) {
        cout << "open failed: " << strerror(errno) << endl;
    }
    int fd_50 = open("/sys/fs/cgroup/two-digit-ms/cgroup.procs", O_RDWR | O_APPEND);
    if(fd_50 == -1) {
        cout << "open failed: " << strerror(errno) << endl;
    }
    int fd_5 = open("/sys/fs/cgroup/single-digit-ms/cgroup.procs", O_RDWR | O_APPEND);
    if(fd_5 == -1) {
        cout << "open failed: " << strerror(errno) << endl;
    }

    cout << "parent pid is " << getpid() << endl;

    global_start = high_resolution_clock::now();
    
    // start cpu reading, give a second to generate a baseline
    // thread measure(readCPU);
    // sleep(1);

    std::vector<pid_t> procs;

    bool is_parent = false;
    
    for(int i=0; i<(NUM_5_MS_SLA + NUM_50_MS_SLA + NUM_500_MS_SLA); i++) {

        int fd_to_use;
        int type;
        // if (i < NUM_500_MS_SLA) {
        //     fd_to_use = fd_500;
        //     type = 500;
        // } else if (i < (NUM_500_MS_SLA + NUM_50_MS_SLA)) {
        //     fd_to_use = fd_50;
        //     type = 50;
        // } else {
        //     fd_to_use = fd_5;
        //     type = 5;
        // }
        if (i < NUM_5_MS_SLA) {
            fd_to_use = fd_5;
            type = 5;
        } else if (i < (NUM_5_MS_SLA + NUM_50_MS_SLA)) {
            fd_to_use = fd_50;
            type = 50;
        } else {
            fd_to_use = fd_500;
            type = 500;
        }

        int c_pid = fork();
  
        if (c_pid == -1) { 
            cout << "fork failed: " << strerror(errno) << endl;
            perror("fork"); 
            exit(EXIT_FAILURE); 
        } else if (c_pid > 0) { 
                    
            long j;
            if (sched_getaffinity(0, sizeof(cpu_set_t), &mask_2) == -1) {
                perror("sched_getaffinity");
            }
            cout << "parent affinity after fork: ";
            for (j = 0; j < nproc; j++) {
                cout << CPU_ISSET(j, &mask_2) << " ";
            }
            cout << endl;

            is_parent = true;
            cout << "parent: child pid is " << c_pid << " with type " << type << endl;
            // if parent, write child pid to cgroup
            string to_write = to_string(c_pid) + "\n";
            size_t nb = write(fd_to_use, to_write.c_str(), to_write.size());
            if (nb == -1) {
                perror("Error writing");
            }
            // add pid to list
            procs.push_back(c_pid);
            // write start time to file
            ofstream file;
            file.open("../worker_out.txt", ios::app);
            file << timeDiff() << ", " << i << ", " << type << ", 0" << endl;
            file.close();
        } else {
            long j;
            if (sched_getaffinity(0, sizeof(cpu_set_t), &mask_2) == -1) {
                perror("sched_getaffinity");
            }
            cout << "child affinity after fork: ";
            for (j = 0; j < nproc; j++) {
                cout << CPU_ISSET(j, &mask_2) << " ";
            }
            cout << endl;

            is_parent = false;
            // if child, execute intense compute
            comp_intense(i, type);
            // break from loop
            break;
        }   
    }
    if (is_parent) {
        close(fd_500);
        close(fd_50);
        close(fd_5);

        ifstream f500("/sys/fs/cgroup/three-digit-ms/cgroup.procs");
        if (f500.is_open()) {
            cout << "pid " << getpid() << ": 500 procs file: " << f500.rdbuf();
        } else {
            cout << "500 file not open?" << endl;
        }
        ifstream f50("/sys/fs/cgroup/two-digit-ms/cgroup.procs");
        if (f50.is_open()) {
            cout << "pid " << getpid() << ": 50 procs file: " << f50.rdbuf();
        } else {
            cout << "50 file not open?" << endl;
        }
        ifstream f5("/sys/fs/cgroup/single-digit-ms/cgroup.procs");
        if (f5.is_open()) {
            cout << "pid " << getpid() << ": 5 procs file: " << f5.rdbuf();
        } else {
            cout << "5 file not open?" << endl;
        }

        for(auto& c_pid : procs){ 
            waitpid(c_pid, NULL, 0);
        }
    }
}

