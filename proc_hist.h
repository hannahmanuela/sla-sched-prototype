#include <map>
#include <iostream>
#include <cstring>
using namespace std;

#include "consts.h"

#ifndef PROC_HIST_H
#define PROC_HIST_H

// the histogram of processes on a given machine
class ProcHist {

    public:
        // map of sla range bottom to tuple< num active procs, num procs in holdq>
        std::map<int, std::tuple<int, int>> hist;

        std::tuple<bool, int> get(int sla) {
            int index = map_sla_to_bottom(sla);
            if (hist.count(index) > 0) {
                return std::make_tuple(false, -1);
            } else {
                return hist[index];
            }
        }

        void put(bool is_active, int sla) {
            // don't need to add procs that are under the threshold of just begin sprayed anyway
            if (sla < THRESHOLD_SPRAY_SLA) {
                return;
            }

            int index = map_sla_to_bottom(sla);
            if (hist.count(index) > 0) {
                if (is_active) {
                    hist[index] = make_tuple(1, 0);
                } else {
                    hist[index] = make_tuple(0, 1);
                }
            } else {
                if (is_active) {
                    std::get<0>(hist[index]) += 1;
                } else {
                    std::get<1>(hist[index]) += 1;
                }
            }
        }

        int map_sla_to_bottom(int sla) {
            if (sla < 5) {
                return 0;
            } else if (sla < 10) {
                return 5;
            } else if (sla < 50) {
                return 10;
            } else if (sla < 500) {
                return 50;
            } else {
                return 500;
            }
        }

        void print() {
            for (auto const& x : hist) {
                cout << "sla: " << x.first << ", active: " << std::get<0>(x.second) << ", hold: " << std::get<1>(x.second) << endl;
            }
        }

        void from_bytes(char* buffer) {
            int hist_size;
            std::memcpy(&hist_size, buffer, sizeof(int));
            buffer += sizeof(int);
            for (int i = 0; i < hist_size; ++i) {
                int index;
                int num_active_procs;
                int num_hold_procs;
                std::memcpy(&index, buffer, sizeof(int));
                buffer += sizeof(int);
                std::memcpy(&num_active_procs, buffer, sizeof(int));
                buffer += sizeof(int);
                std::memcpy(&num_hold_procs, buffer, sizeof(int));
                buffer += sizeof(int);
                hist[index] = std::make_tuple(num_active_procs, num_hold_procs);
            }
        }
        void to_bytes(char* buffer) {
            int hist_size = hist.size();
            std::memcpy(buffer, &hist_size, sizeof(int));
            buffer += sizeof(int);
            for (auto const& x : hist) {
                int index = x.first;
                int num_active_procs = std::get<0>(x.second);
                int num_hold_procs = std::get<1>(x.second);
                std::memcpy(buffer, &index, sizeof(int));
                buffer += sizeof(int);
                std::memcpy(buffer, &num_active_procs, sizeof(int));
                buffer += sizeof(int);
                std::memcpy(buffer, &num_hold_procs, sizeof(int));
                buffer += sizeof(int);
            }
        }
      

};

#endif  // PROC_HIST_H