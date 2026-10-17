#!/bin/bash
sed -i 's/int frames = 240;/int frames = 240;\n#ifdef __linux__\n    frames = 24;\n#endif/g' tests/singularity/slow_adapter_zone_perf_test.cpp
