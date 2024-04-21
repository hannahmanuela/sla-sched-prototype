#include <vector>
#include <iostream>
#include <sys/resource.h>


int main() {

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

    int stime = usage.ru_stime.tv_sec * 1000.0 + (usage.ru_stime.tv_usec/1000.0);
    int utime = usage.ru_utime.tv_sec * 1000.0 + (usage.ru_utime.tv_usec/1000.0);
    std::cout << "CHILD: stime " << stime << ", utime: " << utime <<  std::endl;

    return 0;
           
}