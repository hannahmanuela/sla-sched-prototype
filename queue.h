#include <vector>
#include <algorithm>
#include <mutex>

#include "proc.h"

#ifndef QUEUE_H
#define QUEUE_H


class Queue {

    public:
        Queue() {
            std::vector<Proc*> q_;
            std::mutex lock_;
        };

        // this is ok I think because when I loop it is evaluated only once (so loop may become stale while its running but vec won't change under its feet)
        // https://stackoverflow.com/questions/16259574/c11-range-based-for-loops-evaluate-once-or-multiple-times
        std::vector<Proc*> get_q() {
            lock_.lock();
            std::vector<Proc*> curr_q = q_; 
            lock_.unlock();
            return curr_q;
        }

        void enq(Proc* to_add) {
            lock_.lock();
            q_.push_back(to_add); 
            lock_.unlock();
        }

        void remove(Proc* to_del) { 
            lock_.lock();
            q_.erase(std::remove(q_.begin(), q_.end(), to_del), q_.end());
            lock_.unlock();
        }

    private:
        std::vector<Proc*> q_;
        std::mutex lock_;


};

#endif  // QUEUE_H