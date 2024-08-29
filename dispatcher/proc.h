#include <vector>
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

        Proc(int id_given, float deadline, float comp_ceil, float mem_usg, ProcType type, long long start_time, pthread_t thread) 
            : id(id_given), deadline_(deadline), expected_mem_usg_(mem_usg), time_spawned_(start_time), comp_ceil_(comp_ceil), type_(type), thread_(thread) {}
        
        ~Proc() {
        }

        float get_slack() {
            // time to absolute deadline - (max) time left on comp
            long long abs_deadline = time_spawned_ + (long long) deadline_;
            int time_to_dl = (int)(abs_deadline - get_curr_time_ms());
            return time_to_dl - get_expected_comp_left();
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
        pthread_t thread_;

    private:

        float time_gotten() {
            clockid_t thread_clock_id;
            struct timespec curr_time;
            if (pthread_getcpuclockid(thread_, &thread_clock_id) != 0) {
                return 0.0;
            }
            clock_gettime(thread_clock_id, &curr_time);

            float curr_runtime = (curr_time.tv_sec * 1000.0 + (curr_time.tv_nsec/1000000.0));

            return curr_runtime;
        }

};

#endif  // PROC_H