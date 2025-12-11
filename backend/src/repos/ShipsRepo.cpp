// src/repos/ShipsRepo.cpp
#include "repos/ShipsRepo.h"
#include "db/Db.h"

#include <sqlite3.h>

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

// ---------- RAII wrapper ----------
class Stmt {
public:
    Stmt(sqlite3* db, const char* sql) : db_(db) {
        if (sqlite3_prepare_v2(db_, sql, -1, &st_, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(db_));
        }
    }

    ~Stmt() {
        if (st_) sqlite3_finalize(st_);
    }

    sqlite3_stmt* get() const noexcept { return st_; }

    Stmt(const Stmt&) = delete;
    Stmt& operator=(const Stmt&) = delete;

private:
    sqlite3* db_{nullptr};
    sqlite3_stmt* st_{nullptr};
};

inline std::string safe_text(sqlite3_stmt* st, int col) {
    const unsigned char* t = sqlite3_column_text(st, col);
    return t ? reinterpret_cast<const char*>(t) : "";
}

Ship parseShip(sqlite3_stmt* st) {
    Ship s{};
    s.id         = sqlite3_column_int64(st, 0);
    s.name       = safe_text(st, 1);
    s.type       = safe_text(st, 2);
    s.country    = safe_text(st, 3);
    s.port_id    = sqlite3_column_int64(st, 4); // якщо NULL -> 0 (OK як sentinel)
    s.status     = safe_text(st, 5);
    s.company_id = sqlite3_column_int64(st, 6); // якщо NULL -> 0
    s.speed_knots = sqlite3_column_double(st, 7);
    s.departed_at = safe_text(st, 8);
    s.destination_port_id = sqlite3_column_int64(st, 9);
    s.eta = safe_text(st, 10);
    s.voyage_distance_km = sqlite3_column_double(st, 11);
    return s;
}

inline void bindNullableInt64(sqlite3_stmt* st, int idx, std::int64_t v) {
    if (v > 0) {
        sqlite3_bind_int64(st, idx, v);
    } else {
        sqlite3_bind_null(st, idx);
    }
}

} // namespace

// ===================== ALL =====================

std::vector<Ship> ShipsRepo::all() {
    sqlite3* db = Db::instance().handle();

    const char* sql =
        "SELECT id,name,type,country,port_id,status,IFNULL(company_id,0),"
        "IFNULL(speed_knots,20.0),"
        "departed_at,IFNULL(destination_port_id,0),eta,IFNULL(voyage_distance_km,0) "
        "FROM ships "
        "ORDER BY id";

    Stmt st(db, sql);

    std::vector<Ship> result;
    while (sqlite3_step(st.get()) == SQLITE_ROW) {
        result.push_back(parseShip(st.get()));
    }

    return result;
}

// ===================== BY PORT =====================

std::vector<Ship> ShipsRepo::getByPortId(long long portId) {
    sqlite3* db = Db::instance().handle();

    const char* sql =
        "SELECT id,name,type,country,port_id,status,IFNULL(company_id,0),"
        "IFNULL(speed_knots,20.0),"
        "departed_at,IFNULL(destination_port_id,0),eta,IFNULL(voyage_distance_km,0) "
        "FROM ships "
        "WHERE port_id=? "
        "ORDER BY id";

    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, static_cast<std::int64_t>(portId));

    std::vector<Ship> result;
    while (sqlite3_step(st.get()) == SQLITE_ROW) {
        result.push_back(parseShip(st.get()));
    }

    return result;
}

// ===================== BY ID =====================

std::optional<Ship> ShipsRepo::byId(long long id) {
    sqlite3* db = Db::instance().handle();

    const char* sql =
        "SELECT id,name,type,country,port_id,status,IFNULL(company_id,0),"
        "IFNULL(speed_knots,20.0),"
        "departed_at,IFNULL(destination_port_id,0),eta,IFNULL(voyage_distance_km,0) "
        "FROM ships "
        "WHERE id=?";

    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, static_cast<std::int64_t>(id));

    if (sqlite3_step(st.get()) == SQLITE_ROW) {
        return parseShip(st.get());
    }

    return std::nullopt;
}

// ===================== CREATE =====================

Ship ShipsRepo::create(const Ship& sIn) {
    sqlite3* db = Db::instance().handle();

    const char* sql =
        "INSERT INTO ships(name, type, country, port_id, status, company_id, speed_knots, "
        "departed_at, destination_port_id, eta, voyage_distance_km) "
        "VALUES(?,?,?,?,?,?,?,?,?,?,?);";

    Stmt st(db, sql);

    sqlite3_bind_text(st.get(), 1, sIn.name.c_str(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.get(), 2, sIn.type.c_str(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.get(), 3, sIn.country.c_str(), -1, SQLITE_TRANSIENT);

    // port_id: 0/не задано -> NULL
    bindNullableInt64(st.get(), 4, static_cast<std::int64_t>(sIn.port_id));

    sqlite3_bind_text(st.get(), 5, sIn.status.c_str(),  -1, SQLITE_TRANSIENT);

    // company_id: 0/не задано -> NULL
    bindNullableInt64(st.get(), 6, static_cast<std::int64_t>(sIn.company_id));

    // speed_knots
    sqlite3_bind_double(st.get(), 7, sIn.speed_knots);

    // Voyage tracking fields
    if (sIn.departed_at.empty()) {
        sqlite3_bind_null(st.get(), 8);
    } else {
        sqlite3_bind_text(st.get(), 8, sIn.departed_at.c_str(), -1, SQLITE_TRANSIENT);
    }
    bindNullableInt64(st.get(), 9, static_cast<std::int64_t>(sIn.destination_port_id));
    if (sIn.eta.empty()) {
        sqlite3_bind_null(st.get(), 10);
    } else {
        sqlite3_bind_text(st.get(), 10, sIn.eta.c_str(), -1, SQLITE_TRANSIENT);
    }
    sqlite3_bind_double(st.get(), 11, sIn.voyage_distance_km);

    const int rc = sqlite3_step(st.get());
    if (rc != SQLITE_DONE) {
        throw std::runtime_error(std::string("ShipsRepo::create failed: ") + sqlite3_errmsg(db));
    }

    Ship out = sIn;
    out.id = sqlite3_last_insert_rowid(db);
    try {
        std::string msg = "Created ship id=" + std::to_string(out.id) + " name='" + out.name + "' type='" + out.type + "'";
        Db::instance().insertLog("INFO", "ship.create", "ship", (int)out.id, "system", msg);
    } catch (...) {
        // ignore logging errors
    }
    return out;
}

// ===================== UPDATE =====================

void ShipsRepo::update(const Ship& s) {
    sqlite3* db = Db::instance().handle();

    const char* sql =
        "UPDATE ships "
        "SET name = ?, type = ?, country = ?, port_id = ?, status = ?, company_id = ?, speed_knots = ?, "
        "departed_at = ?, destination_port_id = ?, eta = ?, voyage_distance_km = ? "
        "WHERE id = ?;";

    Stmt st(db, sql);

    sqlite3_bind_text(st.get(), 1, s.name.c_str(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.get(), 2, s.type.c_str(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.get(), 3, s.country.c_str(), -1, SQLITE_TRANSIENT);

    bindNullableInt64(st.get(), 4, static_cast<std::int64_t>(s.port_id));

    sqlite3_bind_text(st.get(), 5, s.status.c_str(),  -1, SQLITE_TRANSIENT);

    bindNullableInt64(st.get(), 6, static_cast<std::int64_t>(s.company_id));

    // speed_knots
    sqlite3_bind_double(st.get(), 7, s.speed_knots);

    // Voyage tracking fields
    if (s.departed_at.empty()) {
        sqlite3_bind_null(st.get(), 8);
    } else {
        sqlite3_bind_text(st.get(), 8, s.departed_at.c_str(), -1, SQLITE_TRANSIENT);
    }
    bindNullableInt64(st.get(), 9, static_cast<std::int64_t>(s.destination_port_id));
    if (s.eta.empty()) {
        sqlite3_bind_null(st.get(), 10);
    } else {
        sqlite3_bind_text(st.get(), 10, s.eta.c_str(), -1, SQLITE_TRANSIENT);
    }
    sqlite3_bind_double(st.get(), 11, s.voyage_distance_km);

    sqlite3_bind_int64(st.get(), 12, static_cast<std::int64_t>(s.id));

    const int rc = sqlite3_step(st.get());
    if (rc != SQLITE_DONE) {
        throw std::runtime_error(std::string("ShipsRepo::update failed: ") + sqlite3_errmsg(db));
    }
    try {
        std::string msg = "Updated ship id=" + std::to_string(s.id) + " name='" + s.name + "' status='" + s.status + "'";
        Db::instance().insertLog("INFO", "ship.update", "ship", (int)s.id, "system", msg);
    } catch (...) {}
}

// ===================== REMOVE =====================

void ShipsRepo::remove(long long id) {
    sqlite3* db = Db::instance().handle();

    const char* sql =
        "DELETE FROM ships WHERE id=?;";

    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, static_cast<std::int64_t>(id));

    const int rc = sqlite3_step(st.get());
    if (rc != SQLITE_DONE) {
        throw std::runtime_error(std::string("ShipsRepo::remove failed: ") + sqlite3_errmsg(db));
    }

    // Семантика "void remove" збережена:
    // якщо id не існує — 0 changes без винятку.
    try {
        std::string msg = "Deleted ship id=" + std::to_string(id);
        Db::instance().insertLog("INFO", "ship.delete", "ship", (int)id, "system", msg);
    } catch (...) {}
}
