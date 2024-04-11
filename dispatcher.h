#include <string.h>
#include <iostream>
#include <fcntl.h>
#include <linux/sched.h>
#include <sched.h>
#include <chrono>
#include <ctime>
using namespace std;

#include "queue.h"

#ifndef DISPATCHER_H
#define DISPATCHER_H


class Dispatcher {

    public:
        int id;

        Dispatcher(std::vector<ProcTypeProfile*> proc_type_profiles, int id_given) {
            id = id_given;
            proc_type_profiles_ = proc_type_profiles;
            active_q_ = new Queue();
            hold_q_ = new Queue();

            // open cgroup files
            fd_one_digit_ms_cgroup_ = open("/sys/fs/cgroup/three-digit-ms", O_RDONLY); // given cpu.weight 10000
            if(fd_one_digit_ms_cgroup_ == -1) {
                cout << "open failed: " << strerror(errno) << endl;
            }
            fd_two_digit_ms_cgroup_ = open("/sys/fs/cgroup/two-digit-ms", O_RDONLY); // given cpu.weight 100
            if(fd_two_digit_ms_cgroup_ == -1) {
                cout << "open failed: " << strerror(errno) << endl;
            }
            fd_three_digit_ms_cgroup_ = open("/sys/fs/cgroup/one-digit-ms", O_RDONLY); // given cpu.weight 1
            if(fd_three_digit_ms_cgroup_ == -1) {
                cout << "open failed: " << strerror(errno) << endl;
            }

            cout << "inited fds: " << fd_one_digit_ms_cgroup_ << ", " << fd_two_digit_ms_cgroup_ << ", " << fd_three_digit_ms_cgroup_ << endl;
        };

        ProcTypeProfile* get_profile (ProcType type) { 
            for (auto profile : proc_type_profiles_) {
                if (profile->type == type) {
                    return profile;
                }
            }
            return nullptr;
        }

        void run();

        int get_one_digit_ms_fd() {return fd_one_digit_ms_cgroup_;}


    private:
        Queue* active_q_;
        Queue* hold_q_;
        int fd_one_digit_ms_cgroup_;
        int fd_two_digit_ms_cgroup_;
        int fd_three_digit_ms_cgroup_;

        std::vector<ProcTypeProfile*> proc_type_profiles_;

        std::chrono::high_resolution_clock::time_point start_time_;

        void create_run_lb_conn_();
        void run_lb_conn_(int lb_conn_fd);

        void add_run_active_proc_(Proc* to_add);
        void add_hold_proc_(Proc* to_add);
        void run_holdq_();

        int time_since_start_();

};

#endif  // DISPATCHER_H