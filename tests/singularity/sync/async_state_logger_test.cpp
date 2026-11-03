#include "Singularity/Foreign/Sync/AsyncStateLogger.hpp"
#include <cassert>
#include <iostream>

int main() {
    std::cout << "Testing AsyncStateLogger...\n";
    {
        AsyncStateLogger logger;
        logger.logEvent("entity1", "pos_x", "10.0", 1.0);
        logger.logEvent("entity2", "pos_y", "20.0", 1.5);
        logger.flush();
    }
    std::cout << "AsyncStateLogger tests passed!\n";
    return 0;
}
