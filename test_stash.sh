#!/bin/bash
git restore --staged .
git stash push -m "perf" src/Singularity/Core/Logger.cpp src/Singularity/Core/Logger.hpp
g++ -O3 -std=c++17 benchmark_enqueue.cpp src/Singularity/Core/Logger.cpp -I. -Isrc -o benchmark_enqueue -pthread
./benchmark_enqueue
git stash pop
g++ -O3 -std=c++17 benchmark_enqueue.cpp src/Singularity/Core/Logger.cpp -I. -Isrc -o benchmark_enqueue -pthread
./benchmark_enqueue
