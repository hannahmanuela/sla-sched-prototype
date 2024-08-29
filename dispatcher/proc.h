#include <vector>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/syscall.h>
#include <string>
#include <chrono>
#include <time.h>

#include "proc_type.h"
#include "utils.h"

#ifndef PROC_H
#define PROC_H

// dispatchers representation of procs
class Proc {

    public:
        Proc() {}

        Proc(int id_given, float deadline, float comp_ceil, float mem_usg, ProcType type, long long start_time, pid_t thread) 
            : id(id_given), deadline_(deadline), expected_mem_usg_(mem_usg), time_spawned_(start_time), comp_ceil_(comp_ceil), type_(type), tid_(thread) {}
        
        ~Proc() {
        }

        float get_slack() {
            // OG slack - time already spent waiting
            int og_slack = deadline_ - comp_ceil_;
            return og_slack - wait_time();
        }

        float time_gotten() {
            // time passed - time waiting
            return (int)(get_curr_time_ms() - time_spawned_) - wait_time();
        }

        float get_expected_comp_left() {     
            return comp_ceil_ - time_gotten();
        }
        
        int id;
        float deadline_; // in ms
        float comp_ceil_; // in ms
        float expected_mem_usg_;
        long long time_spawned_; // in ms, from global start time
        ProcType type_;
        pid_t tid_;

        // Function to get the waiting time in milliseconds for a specific thread
        long long wait_time() {
            // Construct the path to the thread's schedstat file
            std::ostringstream schedstat_filepath;
            schedstat_filepath << "/proc/" << tid_ << "/schedstat";

            // Open the schedstat file
            std::ifstream schedstat_file(schedstat_filepath.str());
            if (!schedstat_file.is_open()) {
                std::cerr << "Failed to open schedstat file: " << schedstat_filepath.str() << std::endl;
                return -1;
            }

            // Read the necessary fields from the schedstat file
            std::string line;
            std::getline(schedstat_file, line);
            schedstat_file.close();

            std::istringstream iss(line);
            long long run_time_ns = 0, wait_time_ns = 0;
            int switches = 0;

            iss >> run_time_ns >> wait_time_ns >> switches;

            // Convert waiting time from nanoseconds to milliseconds
            long long wait_time_ms = wait_time_ns / 1000000;

            return wait_time_ms;
        }

    
    private:

};

#endif  // PROC_H