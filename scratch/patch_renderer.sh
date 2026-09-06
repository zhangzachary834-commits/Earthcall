sed -i '' 's/constexpr bool kHeightGridDdaTraversalVerified = false;/constexpr bool kHeightGridDdaTraversalVerified = true;/g' src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp

sed -i '' 's/const bool gridActive = kHeightGridDdaTraversalVerified \&\& _heightGridDdaEnabled \&\& isProvenHeightfield \&\& hasConservativeHeightGrid \&\& !heightGrid->cells.empty();/const bool gridActive = kHeightGridDdaTraversalVerified \&\& _heightGridDdaEnabled \&\& hasConservativeHeightGrid \&\& !heightGrid->cells.empty();/g' src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp
