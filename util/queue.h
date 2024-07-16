#include <vector>
#include <algorithm>
#include <mutex>

using namespace std;

#include "proc.h"

#ifndef QUEUE_H
#define QUEUE_H


class Queue {

    public:
        Queue() {
            vector<Proc*> q_;
            mutex lock_;
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
            q_.insert(upper_bound(q_.begin(), q_.end(), to_add, [](Proc* a, Proc* b) {return a->get_deadline() < b->get_deadline();}), to_add);
            lock_.unlock();
        }

        bool ok_to_place(float new_deadline, float new_comp_ceil) { 
            lock_.lock();

            float running_wait_time = 0;
            bool inserted = false;
            
            for (auto p : q_) {

                // the new proc would be here: the first time its deadline is larger
                // a little janky, but have to get both the new proc and the one we are currently on in the loop
                if (new_deadline > p->deadline_ && inserted == false) {
                    float new_slack = new_deadline - new_comp_ceil;
                    if (new_slack - running_wait_time < 0.0) {
                        lock_.unlock();
                        cout << "doesn't fit because needed slack is " << running_wait_time << ", but slack in NEW proc is only " << new_slack << endl;
                        return false;
                    }
                    running_wait_time += new_comp_ceil;
                    
                    inserted = true;
                }

                float p_slack = p->get_deadline() - p->get_comp_ceil();

                if (p_slack-running_wait_time < 0.0) {
                    lock_.unlock();
                    cout << "doesn't fit because needed slack is " << running_wait_time << ", but slack in proc is only " << p_slack << endl;
                    return false;
                }
                running_wait_time += p->get_comp_ceil();
            }
            
            lock_.unlock();
            cout << "fits!" << endl;
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


};

#endif  // QUEUE_H