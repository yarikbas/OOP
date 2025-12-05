#include "repos/ShipTypesRepo.h"
#include "db/Db.h"
#include <sqlite3.h>
#include <stdexcept>

static ShipType parse_type(sqlite3_stmt* st) {
    ShipType t;
    t.id   = sqlite3_column_int64(st, 0);
    const unsigned char* c1 = sqlite3_column_text(st, 1);
    const unsigned char* c2 = sqlite3_column_text(st, 2);
    const unsigned char* c3 = sqlite3_column_text(st, 3);
    t.code        = c1 ? reinterpret_cast<const char*>(c1) : "";
    t.name        = c2 ? reinterpret_cast<const char*>(c2) : "";
    t.description = c3 ? reinterpret_cast<const char*>(c3) : "";
    return t;
}

std::vector<ShipType> ShipTypesRepo::all() {
    std::vector<ShipType> out;
    sqlite3* db = Db::instance().handle();
    const char* sql = "SELECT id, code, name, description FROM ship_types ORDER BY id";
    sqlite3_stmt* st{};
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) return out;
    while (sqlite3_step(st) == SQLITE_ROW) out.push_back(parse_type(st));
    sqlite3_finalize(st);
    return out;
}

std::optional<ShipType> ShipTypesRepo::byId(long long id) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "SELECT id, code, name, description FROM ship_types WHERE id=?";
    sqlite3_stmt* st{};
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) return std::nullopt;
    sqlite3_bind_int64(st, 1, id);
    std::optional<ShipType> out;
    if (sqlite3_step(st) == SQLITE_ROW) out = parse_type(st);
    sqlite3_finalize(st);
    return out;
}

std::optional<ShipType> ShipTypesRepo::byCode(const std::string& code) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "SELECT id, code, name, description FROM ship_types WHERE code=?";
    sqlite3_stmt* st{};
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) return std::nullopt;
    sqlite3_bind_text(st, 1, code.c_str(), -1, SQLITE_TRANSIENT);
    std::optional<ShipType> out;
    if (sqlite3_step(st) == SQLITE_ROW) out = parse_type(st);
    sqlite3_finalize(st);
    return out;
}

ShipType ShipTypesRepo::create(const ShipType& t) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "INSERT INTO ship_types(code,name,description) VALUES(?,?,?)";
    sqlite3_stmt* st{};
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK)
        throw std::runtime_error(std::string("prepare failed: ") + sqlite3_errmsg(db));
    sqlite3_bind_text(st, 1, t.code.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 2, t.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 3, t.description.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(st) != SQLITE_DONE) {
        std::string e = sqlite3_errmsg(db);
        sqlite3_finalize(st);
        throw std::runtime_error("insert failed: " + e);
    }
    sqlite3_finalize(st);
    auto id = sqlite3_last_insert_rowid(db);
    auto got = byId(id);
    if (!got) throw std::runtime_error("insert ok but fetch failed");
    return *got;
}

void ShipTypesRepo::update(const ShipType& t) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "UPDATE ship_types SET code=?, name=?, description=? WHERE id=?";
    sqlite3_stmt* st{};
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK)
        throw std::runtime_error(std::string("prepare failed: ") + sqlite3_errmsg(db));
    sqlite3_bind_text(st, 1, t.code.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 2, t.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 3, t.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(st, 4, t.id);
    if (sqlite3_step(st) != SQLITE_DONE) {
        std::string e = sqlite3_errmsg(db);
        sqlite3_finalize(st);
        throw std::runtime_error("update failed: " + e);
    }
    sqlite3_finalize(st);
}

void ShipTypesRepo::remove(long long id) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "DELETE FROM ship_types WHERE id=?";
    sqlite3_stmt* st{};
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK)
        throw std::runtime_error(std::string("prepare failed: ") + sqlite3_errmsg(db));
    sqlite3_bind_int64(st, 1, id);
    if (sqlite3_step(st) != SQLITE_DONE) {
        std::string e = sqlite3_errmsg(db);
        sqlite3_finalize(st);
        throw std::runtime_error("delete failed: " + e);
    }
    sqlite3_finalize(st);
}
