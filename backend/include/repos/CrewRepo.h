#pragma once
#include <string>
#include <vector>
#include <optional>

struct CrewAssignment {
    long long id = 0;
    long long person_id = 0;
    long long ship_id = 0;
    std::string start_utc;                       // ISO-8601 у UTC
    std::optional<std::string> end_utc;          // null => активне
};

class CrewRepo {
public:
    std::vector<CrewAssignment> currentCrewByShip(long long shipId);
    std::optional<CrewAssignment> assign(long long personId, long long shipId, const std::string& startUtc);
    bool endActiveByPerson(long long personId, const std::string& endUtc);
};
