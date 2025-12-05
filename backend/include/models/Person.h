#pragma once
#include <string>
#include <cstdint>

struct Person {
    std::int64_t id = 0;
    std::string  full_name;   // NOT NULL
    std::string  rank;        // наприклад "Captain", "Mate"
    int          active = 1;  // 1|0
};
