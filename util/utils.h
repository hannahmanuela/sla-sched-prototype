#include <chrono>

#ifndef UTILS_H
#define UTILS_H


long long get_curr_time_ms() {
    auto epoch = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();
}


#endif // UTILS_H