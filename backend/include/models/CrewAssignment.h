#pragma once
#include <cstdint>
#include <string>
#include <optional>

struct CrewAssignment {
    std::int64_t id = 0;
    std::int64_t person_id = 0;
    std::int64_t ship_id = 0;
    std::string  start_date;                // YYYY-MM-DD
    std::optional<std::string> end_date;    // null => активне
};
