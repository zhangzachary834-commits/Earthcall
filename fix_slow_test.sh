#!/bin/bash
sed -i 's/TestSupport::BootedEngineHarness h;/TestSupport::BootedEngineHarness h("Player", "humanoid", false);/g' tests/singularity/slow_adapter_zone_perf_test.cpp
