// include/repos/CompaniesRepo.h
#pragma once

#include "models/Company.h"
#include "models/Port.h"
#include "models/Ship.h"

#include <vector>
#include <optional>
#include <string>
#include <cstdint>

class CompaniesRepo {
public:
    // ---- CRUD companies ----
    std::vector<Company> all();
    std::optional<Company> byId(std::int64_t id);

    // старий API
    Company create(const std::string& name);
    bool update(std::int64_t id, const std::string& name);
    bool remove(std::int64_t id);

    // ✅ новий API під тести
    Company create(const Company& c);
    bool update(const Company& c);

    // ---- company ↔ ports ----
    std::vector<Port> ports(std::int64_t companyId);
    bool addPort(std::int64_t companyId, std::int64_t portId, bool isMain);
    bool removePort(std::int64_t companyId, std::int64_t portId);

    // ---- company ↔ ships ----
    std::vector<Ship> ships(std::int64_t companyId);
};
