#include "src/Singularity/Core/Logger.hpp"
#include <iostream>
#include <vector>
#include <unordered_map>
#include <chrono>

using namespace ECA;

int main() {
    std::unordered_map<LogCategory, int> map;
    int arr[6] = {0};

    for (int i = 0; i < 6; ++i) {
        map[static_cast<LogCategory>(i)] = i;
        arr[i] = i;
    }

    auto start_map = std::chrono::high_resolution_clock::now();
    long long sum_map = 0;
    for (int i = 0; i < 100000000; ++i) {
        auto it = map.find(static_cast<LogCategory>(i % 6));
        if (it != map.end()) sum_map += it->second;
    }
    auto end_map = std::chrono::high_resolution_clock::now();

    auto start_arr = std::chrono::high_resolution_clock::now();
    long long sum_arr = 0;
    for (int i = 0; i < 100000000; ++i) {
        sum_arr += arr[static_cast<int>(static_cast<LogCategory>(i % 6))];
    }
    auto end_arr = std::chrono::high_resolution_clock::now();

    std::cout << "Map find sum: " << sum_map << " time: " << std::chrono::duration<double>(end_map - start_map).count() << "s\n";
    std::cout << "Arr idx sum: " << sum_arr << " time: " << std::chrono::duration<double>(end_arr - start_arr).count() << "s\n";

    return 0;
}
