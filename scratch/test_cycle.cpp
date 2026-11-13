#include <iostream>
int main() {
#if defined(__clang__) || defined(__GNUC__)
    std::cout << "Cycle: " << __builtin_readcyclecounter() << std::endl;
#else
    std::cout << "Not supported" << std::endl;
#endif
    return 0;
}
