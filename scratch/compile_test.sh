cd build
g++ -std=c++20 -I../src -I../third_party/nlohmann ../tests/singularity/save_system_error_test.cpp -c || exit 1
echo "Compile OK"
