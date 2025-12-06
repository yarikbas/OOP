#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "models/Company.h"
#include "models/Port.h"
#include "models/Ship.h"

class CompaniesRepo {
public:
    using Id = std::int64_t;

    std::vector<Company> all();
    std::optional<Company> byId(Id id);
    Company create(const std::string& name);
    bool update(Id id, const std::string& name);
    bool remove(Id id);

    // офіси компанії
    std::vector<Port> ports(Id companyId);
    bool addPort(Id companyId, Id portId, bool isHq);
    bool removePort(Id companyId, Id portId);

    // кораблі компанії
    std::vector<Ship> ships(Id companyId);
};
