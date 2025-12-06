#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "models/Ship.h"

class ShipsRepo {
public:
    using Id = std::int64_t;

    std::vector<Ship> all();
    std::vector<Ship> getByPortId(Id portId);

    Ship create(const Ship& s);
    std::optional<Ship> byId(Id id);
    void update(const Ship& s);
    void remove(Id id);
};
