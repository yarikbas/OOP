#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct CrewAssignment {
    std::int64_t id{0};
    std::int64_t person_id{0};
    std::int64_t ship_id{0};
    std::string start_utc;                      // ISO-8601 у UTC
    std::optional<std::string> end_utc;         // null => активне
};

class CrewRepo {
public:
    using Id = std::int64_t;

    std::vector<CrewAssignment> currentCrewByShip(Id shipId);
    std::optional<CrewAssignment> assign(Id personId, Id shipId, const std::string& startUtc);
    bool endActiveByPerson(Id personId, const std::string& endUtc);
};
