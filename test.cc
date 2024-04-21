#include <vector>
#include <iostream>
#include <syscall.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <chrono>

#include "consts.h"

using namespace std;

int main() {

    vector<const char*> command;
    command.push_back(FG_PATH);
    command.push_back(NULL); 

    std::chrono::high_resolution_clock::time_point start_time = std::chrono::high_resolution_clock::now();

    int c_pid = fork();

    if (c_pid == -1) { 
        perror("fork"); 
    } else if (c_pid > 0) { 
        // PARENT
        struct rusage usage_stats;
        int ret = wait4(c_pid, NULL, 0, &usage_stats);
        if (ret < 0) {
            perror("wait4 did bad");
        }
        // usec is in microseconds so /1000 in millisec; mem used in KB so /1000 in MB
        // float runtime = (usage_stats.ru_utime.tv_usec + usage_stats.ru_stime.tv_usec)/1000;
        float runtime = (usage_stats.ru_utime.tv_sec * 1000.0 + (usage_stats.ru_utime.tv_usec/1000.0))
                                + (usage_stats.ru_stime.tv_sec * 1000.0 + (usage_stats.ru_stime.tv_usec/1000.0));
        float mem_used = usage_stats.ru_maxrss / 1000;

        std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start_time);
        int time_passed = 1000 * time_span.count();  // 1000 => shows millisec; 1000000 => shows microsec

        cout << "wait4 runtime: " << runtime << ", time passed: " << time_passed << endl;
    } else {
        // if child, execute intense compute
        int ret = execv(command[0], const_cast<char* const*>(command.data()));
        if (ret < 0) {
            perror("ERROR running proc");
        }
    }

}