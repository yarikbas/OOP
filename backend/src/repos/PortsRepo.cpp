#include "repos/PortsRepo.h"
#include <stdexcept>

PortsRepo::PortsRepo()
    : db_(Db::instance().handle()) {}

PortsRepo::PortsRepo(sqlite3* db)
    : db_(db) {}

std::vector<Port> PortsRepo::all() const {
    std::vector<Port> result;

    sqlite3_stmt* st{};
    const char* sql =
        "SELECT id, name, region, lat, lon "
        "FROM ports "
        "ORDER BY id;";

    if (sqlite3_prepare_v2(db_, sql, -1, &st, nullptr) != SQLITE_OK) {
        throw std::runtime_error("PortsRepo::all prepare failed");
    }

    while (sqlite3_step(st) == SQLITE_ROW) {
        Port p;
        p.id     = sqlite3_column_int64(st, 0);
        p.name   = reinterpret_cast<const char*>(sqlite3_column_text(st, 1));
        p.region = reinterpret_cast<const char*>(sqlite3_column_text(st, 2));
        p.lat    = sqlite3_column_double(st, 3);
        p.lon    = sqlite3_column_double(st, 4);
        result.push_back(std::move(p));
    }

    sqlite3_finalize(st);
    return result;
}

Port PortsRepo::create(const Port& in) const {
    sqlite3_stmt* st{};
    const char* sql =
        "INSERT INTO ports (name, region, lat, lon) "
        "VALUES (?, ?, ?, ?);";

    if (sqlite3_prepare_v2(db_, sql, -1, &st, nullptr) != SQLITE_OK) {
        throw std::runtime_error("PortsRepo::create prepare failed");
    }

    sqlite3_bind_text (st, 1, in.name.c_str(),   -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (st, 2, in.region.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(st, 3, in.lat);
    sqlite3_bind_double(st, 4, in.lon);

    const int rc = sqlite3_step(st);
    sqlite3_finalize(st);

    if (rc != SQLITE_DONE) {
        throw std::runtime_error("PortsRepo::create step failed");
    }

    Port out = in;
    out.id = sqlite3_last_insert_rowid(db_);
    return out;
}

std::optional<Port> PortsRepo::getById(int64_t id) const {
    sqlite3_stmt* st{};
    const char* sql =
        "SELECT id, name, region, lat, lon "
        "FROM ports "
        "WHERE id = ?;";

    if (sqlite3_prepare_v2(db_, sql, -1, &st, nullptr) != SQLITE_OK) {
        throw std::runtime_error("PortsRepo::getById prepare failed");
    }

    sqlite3_bind_int64(st, 1, id);

    int rc = sqlite3_step(st);
    if (rc == SQLITE_ROW) {
        Port p;
        p.id     = sqlite3_column_int64(st, 0);
        p.name   = reinterpret_cast<const char*>(sqlite3_column_text(st, 1));
        p.region = reinterpret_cast<const char*>(sqlite3_column_text(st, 2));
        p.lat    = sqlite3_column_double(st, 3);
        p.lon    = sqlite3_column_double(st, 4);
        sqlite3_finalize(st);
        return p;
    }

    sqlite3_finalize(st);
    return std::nullopt;
}

bool PortsRepo::update(const Port& p) const {
    sqlite3_stmt* st{};
    const char* sql =
        "UPDATE ports "
        "SET name = ?, region = ?, lat = ?, lon = ? "
        "WHERE id = ?;";

    if (sqlite3_prepare_v2(db_, sql, -1, &st, nullptr) != SQLITE_OK) {
        throw std::runtime_error("PortsRepo::update prepare failed");
    }

    sqlite3_bind_text (st, 1, p.name.c_str(),   -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (st, 2, p.region.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(st, 3, p.lat);
    sqlite3_bind_double(st, 4, p.lon);
    sqlite3_bind_int64 (st, 5, p.id);

    const int rc = sqlite3_step(st);
    sqlite3_finalize(st);

    if (rc != SQLITE_DONE) return false;
    return sqlite3_changes(db_) > 0;
}

bool PortsRepo::remove(int64_t id) const {
    sqlite3_stmt* st{};
    const char* sql = "DELETE FROM ports WHERE id = ?;";

    if (sqlite3_prepare_v2(db_, sql, -1, &st, nullptr) != SQLITE_OK) {
        throw std::runtime_error("PortsRepo::remove prepare failed");
    }

    sqlite3_bind_int64(st, 1, id);
    const int rc = sqlite3_step(st);
    sqlite3_finalize(st);

    if (rc != SQLITE_DONE) return false;
    return sqlite3_changes(db_) > 0;
}
