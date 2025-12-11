// include/repos/CargoRepo.h
#pragma once

#include "models/Cargo.h"
#include <vector>
#include <optional>
#include <cstdint>

class CargoRepo {
public:
    std::vector<Cargo> all();
    std::vector<Cargo> byShipId(std::int64_t shipId);
    std::vector<Cargo> byStatus(const std::string& status);
    std::optional<Cargo> byId(std::int64_t id);
    Cargo create(const Cargo& cargo);
    void update(const Cargo& cargo);
    void remove(std::int64_t id);
};
