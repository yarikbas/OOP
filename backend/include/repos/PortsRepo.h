// include/repos/PortsRepo.h
#pragma once

#include "models/Port.h"

#include <sqlite3.h>
#include <cstdint>
#include <optional>
#include <vector>

class PortsRepo {
public:
    // За замовчуванням бере Db::instance().handle()
    PortsRepo();

    // Для тестів/DI можна передати явний sqlite3*
    explicit PortsRepo(sqlite3* db);

    std::vector<Port> all() const;

    Port create(const Port& in) const;

    std::optional<Port> getById(std::int64_t id) const;

    bool update(const Port& p) const;

    bool remove(std::int64_t id) const;

private:
    sqlite3* db_{nullptr};
};
