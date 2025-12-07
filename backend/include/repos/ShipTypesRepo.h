// include/repos/ShipTypesRepo.h
#pragma once

#include "models/ShipType.h"

#include <optional>
#include <string>
#include <vector>

class ShipTypesRepo {
public:
    std::vector<ShipType> all();

    std::optional<ShipType> byId(long long id);
    std::optional<ShipType> byCode(const std::string& code);

    ShipType create(const ShipType& t);

    void update(const ShipType& t);

    void remove(long long id);
};
