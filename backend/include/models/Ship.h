#pragma once
#include <string>
#include <cstdint>

struct Ship {
    std::int64_t id = 0;
    std::string  name;
    std::string  type;
    std::string  country;
    std::int64_t port_id = 0;
    std::string  status = "docked";
    std::int64_t company_id = 0;   // <--- ДОДАЛИ
};
