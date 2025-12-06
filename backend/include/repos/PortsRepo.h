#pragma once
#include "db/Db.h"
#include "models/Port.h"
#include <vector>
#include <optional>

class PortsRepo {
public:
    PortsRepo();
    explicit PortsRepo(sqlite3* db);

    std::vector<Port> all() const;
    Port create(const Port& p) const;
    std::optional<Port> getById(int64_t id) const;
    bool update(const Port& p) const;
    bool remove(int64_t id) const;

private:
    sqlite3* db_;
};
