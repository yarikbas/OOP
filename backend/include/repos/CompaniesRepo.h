#pragma once
#include "models/Port.h"
#include <vector>
#include <optional>
#include <cstdint>
#include <string>
#include "models/Company.h"
#include "repos/PortsRepo.h"
#include "models/Ship.h"

class CompaniesRepo {
public:
    std::vector<Company> all();
    std::optional<Company> byId(std::int64_t id);
    Company create(const std::string& name);
    bool update(std::int64_t id, const std::string& name);
    bool remove(std::int64_t id);

    // офіси компанії
    std::vector<Port> ports(std::int64_t companyId);
    bool addPort(std::int64_t companyId, std::int64_t portId, bool isHq);
    bool removePort(std::int64_t companyId, std::int64_t portId);

    // кораблі компанії
    std::vector<Ship> ships(std::int64_t companyId);
};

