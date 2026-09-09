#include <cstdlib>
#include <iostream>
#include <string>

int main() {
    const std::string expected = R"("status")";
    if (expected.empty()) {
        std::cerr << "unexpected\n";
        return 1;
    }
    return 0;
}
