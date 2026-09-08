#!/bin/bash
echo "=== Baseline ==="
git stash push -m "perf" src/Singularity/Core/Logger.cpp src/Singularity/Core/Logger.hpp
echo "Compiling..."
g++ -O3 -std=c++17 benchmark_lookup.cpp src/Singularity/Core/Logger.cpp -I. -Isrc -o benchmark_lookup -pthread
g++ -O3 -std=c++17 benchmark_worker.cpp src/Singularity/Core/Logger.cpp -I. -Isrc -o benchmark_worker -pthread
echo "benchmark_lookup:"
./benchmark_lookup
echo "benchmark_worker:"
./benchmark_worker
git stash pop

echo "=== Optimized ==="
echo "Compiling..."
g++ -O3 -std=c++17 benchmark_lookup.cpp src/Singularity/Core/Logger.cpp -I. -Isrc -o benchmark_lookup -pthread
g++ -O3 -std=c++17 benchmark_worker.cpp src/Singularity/Core/Logger.cpp -I. -Isrc -o benchmark_worker -pthread
echo "benchmark_lookup:"
./benchmark_lookup
echo "benchmark_worker:"
./benchmark_worker
