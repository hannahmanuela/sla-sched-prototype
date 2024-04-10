#include <vector>


int main() {

    int to_factor = 964653;
    std::vector<int> factors;

    for (int i = 1; i <= to_factor; i++) {
        if (to_factor % i == 0) {
            factors.push_back(i);
        }
    }

    return 0;
           
}