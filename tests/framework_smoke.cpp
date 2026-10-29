#include "ealr/framework.hpp"

#include <cstring>

int main() {
    if (std::strcmp(ealr::frameworkName(), "Private 5G Energy-Aware Routing") != 0) {
        return 1;
    }

    return std::strlen(ealr::frameworkVersion()) == 0;
}
