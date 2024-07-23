#include <vector>
#include <string>
#include <chrono>
#include <time.h>

#include "proc_type.h"

#ifndef PROC_H
#define PROC_H

// dispatchers representation of procs
class Proc {

    public:
        Proc() {}

        Proc(float deadline, float comp_ceil, ProcType type, std::chrono::high_resolution_clock::time_point start_time, pthread_t thread) 
            : deadline_(deadline), time_spawned_(start_time), comp_ceil_(comp_ceil), type_(type), thread_(thread) {}
        
        ~Proc() {
        }

        float get_deadline() {
            return deadline_;
        }
        float get_comp_ceil() {
            return comp_ceil_;
        }

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

        float get_expected_comp_left() {
            return comp_ceil_ - time_gotten();
        }

        float deadline_; // for now in ms
        float comp_ceil_; // for now in ms
        std::chrono::high_resolution_clock::time_point time_spawned_; // for now in ms, from global start time
        ProcType type_;
        pthread_t thread_;

    private:

};

#endif  // PROC_H