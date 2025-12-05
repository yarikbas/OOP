#pragma once
#include "models/ShipType.h"
#include <vector>
#include <optional>

class ShipTypesRepo {
public:
    std::vector<ShipType> all();
    std::optional<ShipType> byId(long long id);
    std::optional<ShipType> byCode(const std::string& code);
    ShipType create(const ShipType& t);  // повертає створений запис
    void update(const ShipType& t);
    void remove(long long id);
};
