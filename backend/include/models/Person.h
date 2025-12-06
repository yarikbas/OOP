#pragma once

#include <cstdint>
#include <string>

struct Person {
    std::int64_t id{0};
    std::string  full_name;   // NOT NULL
    std::string  rank;        // напр. "Captain", "Engineer" (краще уніфікований code)
    int          active{1};   // 1|0
};
