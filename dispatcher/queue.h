#include <vector>
#include <algorithm>
#include <mutex>
#include <limits>

using namespace std;

#include "proc.h"
#include "consts.h"

#ifndef QUEUE_H
#define QUEUE_H


class Queue {

    public:
        Queue() {
            vector<Proc*> q_;
            mutex lock_;
            num_cores_ = (std::thread::hardware_concurrency() - 2) / 2;
        };

        // this is ok I think because when I loop it is evaluated only once (so loop may become stale while its running but vec won't change under its feet)
        // https://stackoverflow.com/questions/16259574/c11-range-based-for-loops-evaluate-once-or-multiple-times
        vector<Proc*> get_q() {
            lock_.lock();
            vector<Proc*> curr_q = q_; 
            lock_.unlock();
            return curr_q;
        }

        int get_qlen() {
            lock_.lock();
            int curr_qlen = q_.size(); 
            lock_.unlock();
            return curr_qlen;
        }

        void enq(Proc* to_add) {
            lock_.lock();
            q_.insert(upper_bound(q_.begin(), q_.end(), to_add, [](Proc* a, Proc* b) {return a->deadline_ < b->deadline_;}), to_add);
            lock_.unlock();
        }

        bool ok_to_place(float new_deadline, float new_comp_ceil) { 
            lock_.lock();

            float running_wait_time = 0;
            bool inserted = false;

            map<int, float> cores_to_running_waiting_time;
            for (int i = 0; i < num_cores_; i++) {
                cores_to_running_waiting_time.insert({i, (float)0});
            }

            int num_cores = num_cores_;

            auto get_add_min_running_wait_time = [&cores_to_running_waiting_time, &num_cores](float to_add)->float { 
                float min_val = std::numeric_limits<float>::max();
                int min_core = -1;
                for (int i = 0; i < num_cores; i++) {
                    if (cores_to_running_waiting_time.at(i) < min_val) {
                        min_val = cores_to_running_waiting_time.at(i);
                        min_core = i;
                    }
                }
                // cout << "adding ceil " << to_add << " to core " << min_core << ", whose waiting time is thus now " << cores_to_running_waiting_time.at(min_core) + to_add << endl;
                cores_to_running_waiting_time[min_core] += to_add;
                return min_val;
            };
            
            for (auto p : q_) {

                // the new proc would be here: the first time its deadline is larger
                // a little janky, but have to get both the new proc and the one we are currently on in the loop
                if (inserted == false && new_deadline > p->deadline_) {
                    float new_slack = new_deadline - new_comp_ceil;
                    float wait_time = get_add_min_running_wait_time(p->comp_ceil_);
                    if (new_slack - wait_time < 0.0) {
                        lock_.unlock();
                        ofstream sched_file;
                        sched_file.open("../sched.txt", std::ios_base::app);
                        sched_file << "doesn't fit, trying to place dl: " << new_deadline << ", comp ceil: " << new_comp_ceil << " --> needed slack is " << running_wait_time << ", but NEW proc is only " << new_slack << endl;
                        for (auto p : q_) {
                            sched_file << "   id: " << p->id << ", (abs) dl: " << p->time_spawned_ + p->deadline_ << ", (rel) dl: " << p->deadline_ << ", time gotten: " << p->get_expected_comp_left() << endl;
                        }
                        sched_file.close();
                        return false;
                    }
                    
                    inserted = true;
                }

                float wait_time = get_add_min_running_wait_time(p->get_expected_comp_left());
                if (p->get_slack() - wait_time < 0.0) {
                    ofstream sched_file;
                    sched_file.open("../sched.txt", std::ios_base::app);
                    sched_file << "doesn't fit, trying to place dl: " << new_deadline << ", comp ceil: " << new_comp_ceil << " --> needed slack is " << running_wait_time << ", but slack in proc is only " << p->get_slack() << endl;
                    for (auto p : q_) {
                        sched_file << "   id: " << p->id << ", (abs) dl: " << p->time_spawned_ + p->deadline_ << ", (rel) dl: " << p->deadline_ << ", time gotten: " << p->get_expected_comp_left() << endl;
                    }
                    sched_file.close();
                    lock_.unlock();
                    return false;
                }
            }
            
            lock_.unlock();
            return true;
        }

        float get_max_ratio() { 
            // TODO: This
            return 1.0;
        }

        void remove(Proc* to_del) { 
            lock_.lock();
            q_.erase(std::remove(q_.begin(), q_.end(), to_del), q_.end());
            lock_.unlock();
        }


    private:
        vector<Proc*> q_;
        mutex lock_;
        int num_cores_;


};

#endif  // QUEUE_H