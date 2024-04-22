#include <vector>
#include <iostream>
#include <sys/resource.h>
#include <chrono>


int main() {

    std::chrono::high_resolution_clock::time_point start_time = std::chrono::high_resolution_clock::now();

    int to_factor = 43695353;
    std::vector<int> factors;

    for (int i = 1; i <= to_factor; i++) {
        if (to_factor % i == 0) {
            factors.push_back(i);
        }
    }

    struct rusage usage;
    int ret = getrusage(RUSAGE_SELF, &usage);
    if (ret <0) {
        perror("getting rusage?");
    }

     std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start_time);
        int time_passed = 1000 * time_span.count();  // 1000 => shows millisec; 1000000 => shows microsec

    int runtime = usage.ru_stime.tv_sec * 1000.0 + (usage.ru_stime.tv_usec/1000.0) + usage.ru_utime.tv_sec * 1000.0 + (usage.ru_utime.tv_usec/1000.0);
    std::cout << "rusage runtime: " << runtime << ", time passed: " << time_passed << std::endl;

    return 0;
           
}