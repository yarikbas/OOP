#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "models/ShipType.h"

class ShipTypesRepo {
public:
    using Id = std::int64_t;

    std::vector<ShipType> all();
    std::optional<ShipType> byId(Id id);
    std::optional<ShipType> byCode(const std::string& code);
    ShipType create(const ShipType& t);
    void update(const ShipType& t);
    void remove(Id id);
};
