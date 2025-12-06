#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "models/Port.h"

// forward declaration, щоб не тягнути sqlite3.h сюди
struct sqlite3;

class PortsRepo {
public:
    using Id = std::int64_t;

    PortsRepo();
    explicit PortsRepo(sqlite3* db);

    std::vector<Port> all() const;
    Port create(const Port& p) const;
    std::optional<Port> getById(Id id) const;
    bool update(const Port& p) const;
    bool remove(Id id) const;

private:
    sqlite3* db_{nullptr};
};
