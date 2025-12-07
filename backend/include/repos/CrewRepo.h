// include/repos/CrewRepo.h
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// ✅ Модель тут, бо окремого файла немає
struct CrewAssignment {
    std::int64_t id{0};
    std::int64_t person_id{0};
    std::int64_t ship_id{0};
    std::string start_utc{};
    std::optional<std::string> end_utc{};
};

class CrewRepo {
public:
    std::vector<CrewAssignment> currentCrewByShip(long long shipId);

    std::optional<CrewAssignment> assign(long long personId,
                                         long long shipId,
                                         const std::string& startUtc);

    // ✅ для тестів
    bool end(long long assignmentId);
    bool end(long long assignmentId, const std::string& endUtc);

    // те, що вже було
    bool endActiveByPerson(long long personId, const std::string& endUtc);
};
