#pragma once

#include <cstdint>
#include <string>

struct Port {
    std::int64_t id{0};   // PK
    std::string  name;   // унікальна назва
    std::string  region; // Europe/Asia/...
    double       lat{0.0};
    double       lon{0.0};
};
