#include "repos/ShipsRepo.h"
#include "db/Db.h"

#include <stdexcept>

static Ship parseShip(sqlite3_stmt* st) {
    Ship s;
    s.id         = sqlite3_column_int64(st, 0);
    s.name       = reinterpret_cast<const char*>(sqlite3_column_text(st, 1));
    s.type       = reinterpret_cast<const char*>(sqlite3_column_text(st, 2));
    s.country    = reinterpret_cast<const char*>(sqlite3_column_text(st, 3));
    s.port_id    = sqlite3_column_int64(st, 4);
    s.status     = reinterpret_cast<const char*>(sqlite3_column_text(st, 5));
    s.company_id = sqlite3_column_int64(st, 6);
    return s;
}

// Якщо при створенні корабля port_id == 0,
// підбираємо перший існуючий порт з таблиці ports.
static std::int64_t resolvePortId(sqlite3* db, std::int64_t desired) {
    if (desired > 0) return desired;

    sqlite3_stmt* st = nullptr;
    const char* sql = "SELECT id FROM ports ORDER BY id LIMIT 1";
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) {
        return 0;
    }

    std::int64_t out = 0;
    if (sqlite3_step(st) == SQLITE_ROW) {
        out = sqlite3_column_int64(st, 0);
    }
    sqlite3_finalize(st);
    return out;
}

std::vector<Ship> ShipsRepo::all() {
    sqlite3* db = Db::instance().handle();
    const char* sql =
        "SELECT id,name,type,country,port_id,status,company_id "
        "FROM ships ORDER BY id";

    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare SELECT in ShipsRepo::all");
    }

    std::vector<Ship> result;
    while (sqlite3_step(st) == SQLITE_ROW) {
        result.push_back(parseShip(st));
    }
    sqlite3_finalize(st);
    return result;
}

std::optional<Ship> ShipsRepo::byId(std::int64_t id) {
    sqlite3* db = Db::instance().handle();
    const char* sql =
        "SELECT id,name,type,country,port_id,status,company_id "
        "FROM ships WHERE id = ?";

    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare SELECT in ShipsRepo::byId");
    }

    sqlite3_bind_int64(st, 1, id);

    std::optional<Ship> result;
    if (sqlite3_step(st) == SQLITE_ROW) {
        result = parseShip(st);
    }
    sqlite3_finalize(st);
    return result;
}

Ship ShipsRepo::create(const Ship& sIn) {
    sqlite3* db = Db::instance().handle();

    // 1) Визначаємо порт: якщо port_id не вказаний, беремо перший існуючий порт.
    std::int64_t portId = resolvePortId(db, sIn.port_id);
    if (portId <= 0) {
        throw std::runtime_error("No valid port found for new ship");
    }

    // 2) Формуємо INSERT
    const char* sql =
        "INSERT INTO ships(name, type, country, port_id, status, company_id) "
        "VALUES(?,?,?,?,?,?);";

    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare INSERT in ShipsRepo::create");
    }

    sqlite3_bind_text (st, 1, sIn.name.c_str(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (st, 2, sIn.type.c_str(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (st, 3, sIn.country.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(st, 4, portId);
    sqlite3_bind_text (st, 5, sIn.status.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(st, 6, sIn.company_id);

    if (sqlite3_step(st) != SQLITE_DONE) {
        sqlite3_finalize(st);
        throw std::runtime_error("Failed to execute INSERT in ShipsRepo::create");
    }
    sqlite3_finalize(st);

    Ship out = sIn;
    out.id      = sqlite3_last_insert_rowid(db);
    out.port_id = portId;
    return out;
}

void ShipsRepo::update(const Ship& s) {
    sqlite3* db = Db::instance().handle();

    const char* sql =
        "UPDATE ships "
        "SET name = ?, type = ?, country = ?, port_id = ?, status = ?, company_id = ? "
        "WHERE id = ?;";

    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare UPDATE in ShipsRepo::update");
    }

    sqlite3_bind_text (st, 1, s.name.c_str(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (st, 2, s.type.c_str(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (st, 3, s.country.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(st, 4, s.port_id);
    sqlite3_bind_text (st, 5, s.status.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(st, 6, s.company_id);
    sqlite3_bind_int64(st, 7, s.id);

    if (sqlite3_step(st) != SQLITE_DONE) {
        sqlite3_finalize(st);
        throw std::runtime_error("Failed to execute UPDATE in ShipsRepo::update");
    }
    sqlite3_finalize(st);
}

void ShipsRepo::remove(std::int64_t id) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "DELETE FROM ships WHERE id = ?;";

    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare DELETE in ShipsRepo::remove");
    }

    sqlite3_bind_int64(st, 1, id);

    if (sqlite3_step(st) != SQLITE_DONE) {
        sqlite3_finalize(st);
        throw std::runtime_error("Failed to execute DELETE in ShipsRepo::remove");
    }
    sqlite3_finalize(st);
}
