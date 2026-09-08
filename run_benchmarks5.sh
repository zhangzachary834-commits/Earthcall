#!/bin/bash
echo "=== Baseline ==="
git stash push -m "perf" src/Singularity/Core/Logger.cpp src/Singularity/Core/Logger.hpp
g++ -O3 -std=c++17 test_raw_lookup.cpp src/Singularity/Core/Logger.cpp -I. -Isrc -o test_raw_lookup -pthread
./test_raw_lookup
./test_raw_lookup
git stash pop

echo "=== Optimized ==="
g++ -O3 -std=c++17 test_raw_lookup.cpp src/Singularity/Core/Logger.cpp -I. -Isrc -o test_raw_lookup -pthread
./test_raw_lookup
./test_raw_lookup
