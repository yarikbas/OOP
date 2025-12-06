#pragma once
#include "db/Db.h"
#include "repos/PortsRepo.h"
#include "repos/ShipsRepo.h"
#include "repos/PeopleRepo.h"
#include "repos/CompaniesRepo.h"
#include "repos/ShipTypesRepo.h"

inline Port ensurePort(const std::string& name = "TestPort",
                       const std::string& region = "TestRegion",
                       double lat = 1.0, double lon = 2.0)
{
    PortsRepo ports;
    Port p;
    p.name = name;
    p.region = region;
    p.lat = lat;
    p.lon = lon;
    return ports.create(p);
}

inline Company ensureCompany(const std::string& name = "TestCompany")
{
    CompaniesRepo companies;
    Company c;
    c.name = name;
    return companies.create(c);
}

inline Person ensurePerson(const std::string& full_name = "Test Person",
                           const std::string& rank = "Engineer",
                           int active = 1)
{
    PeopleRepo people;
    Person p;
    p.full_name = full_name;
    p.rank = rank;
    p.active = active;
    return people.create(p);
}

inline Ship ensureShip(const std::string& name = "Test Ship",
                       const std::string& type = "cargo",
                       const std::string& country = "UA",
                       std::int64_t port_id = 0,
                       const std::string& status = "docked",
                       std::int64_t company_id = 0)
{
    ShipsRepo ships;
    Ship s;
    s.name = name;
    s.type = type;
    s.country = country;
    s.port_id = port_id; // 0 дозволяємо - repo сам resolve
    s.status = status;
    s.company_id = company_id;
    return ships.create(s);
}

inline ShipType ensureShipType(const std::string& code = "test_code",
                               const std::string& name = "Test Type",
                               const std::string& description = "Desc")
{
    ShipTypesRepo types;
    ShipType t;
    t.code = code;
    t.name = name;
    t.description = description;
    return types.create(t);
}
