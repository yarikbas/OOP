// include/repos/VoyageRecordsRepo.h
#pragma once

#include "models/VoyageRecord.h"
#include <vector>
#include <optional>
#include <cstdint>

class VoyageRecordsRepo {
public:
    std::vector<VoyageRecord> all();
    std::vector<VoyageRecord> byShipId(std::int64_t shipId);
    std::optional<VoyageRecord> byId(std::int64_t id);
    VoyageRecord create(const VoyageRecord& record);
    void update(const VoyageRecord& record);
    void remove(std::int64_t id);
};
