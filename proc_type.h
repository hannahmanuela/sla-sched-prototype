#include <vector>
#include <string>
#include <cmath>

#ifndef DISTRBUTION_H
#define DISTRBUTION_H

class Distribution {

    public:
        double avg; 
        double std_dev;
        Distribution(){}

        Distribution(float init_avg) {
            avg = init_avg;
            std_dev = 0;
            count_ = 1;
        }

        void update(double new_val) {
            avg = (avg*float(count_) + new_val) / float(count_+1);
            std_dev = std::sqrt((std::pow(std_dev, 2) * static_cast<double>(count_) + std::pow(new_val - avg, 2)) / static_cast<double>(count_ + 1));
        }
        

    private:
        int count_;
    

};

#endif  // DISTRBUTION_H

#ifndef PROC_TYPE_PROFILE_H
#define PROC_TYPE_PROFILE_H

enum ProcType { static_page_get, dynamic_page_get, data_process_fg, data_process_bg };

class ProcTypeProfile {

    public:
        ProcType type;
        Distribution* mem;
        Distribution* compute;

        ProcTypeProfile(float init_mem, float init_compute) {
            mem = new Distribution(init_mem);
            compute = new Distribution(init_compute);
        };


    private:
    

};

#endif  // PROC_TYPE_PROFILE_H