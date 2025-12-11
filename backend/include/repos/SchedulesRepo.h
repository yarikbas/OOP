// include/repos/SchedulesRepo.h
#pragma once

#include "models/Schedule.h"
#include <vector>
#include <optional>
#include <cstdint>

class SchedulesRepo {
public:
    std::vector<Schedule> all();
    std::vector<Schedule> byShipId(std::int64_t shipId);
    std::vector<Schedule> active();
    std::optional<Schedule> byId(std::int64_t id);
    Schedule create(const Schedule& schedule);
    void update(const Schedule& schedule);
    void remove(std::int64_t id);
};
