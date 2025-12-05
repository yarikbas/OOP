#pragma once
#include "models/Ship.h"
#include <vector>
#include <optional>

class ShipsRepo {
public:
    std::vector<Ship> all();
    std::vector<Ship> getByPortId(long long portId);

    Ship create(const Ship& s);
    std::optional<Ship> byId(long long id);
    void update(const Ship& s);
    void remove(long long id);
};
