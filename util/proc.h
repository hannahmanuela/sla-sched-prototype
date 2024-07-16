#include <vector>
#include <string>
#include <chrono>

#include "proc_type.h"

#ifndef PROC_H
#define PROC_H

// dispatchers representation of procs
class Proc {

    public:
        Proc() {}

        Proc(float deadline, float comp_ceil, ProcType type, std::chrono::high_resolution_clock::time_point start_time) 
            : deadline_(deadline), time_spawned_(start_time), comp_ceil_(comp_ceil), type_(type) {}
        
        ~Proc() {
        }

        float get_deadline() {
            return deadline_;
        }
        float get_comp_ceil() {
            return comp_ceil_;
        }

        float deadline_; // for now in ms
        float comp_ceil_; // for now in ms
        std::chrono::high_resolution_clock::time_point time_spawned_; // for now in ms, from global start time
        ProcType type_;

    private:

};

#endif  // PROC_H