#!/bin/bash
sed -i 's/${TEST_NAME} STREQUAL "synthesis_studio_app_test" OR/             ${TEST_NAME} STREQUAL "slow_adapter_zone_perf_test" OR\n            ${TEST_NAME} STREQUAL "synthesis_studio_app_test" OR/g' CMakeLists.txt
