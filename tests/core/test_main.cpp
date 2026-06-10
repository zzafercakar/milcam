// tests/core/test_main.cpp
#include "check.h"
int main() {
    for (auto& c : t::cases()) c.fn();
    if (t::fails() == 0) { std::printf("ALL PASS (%zu cases)\n", t::cases().size()); return 0; }
    std::printf("%d failure(s)\n", t::fails());
    return 1;
}
