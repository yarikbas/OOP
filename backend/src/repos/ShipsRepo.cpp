#include "repos/ShipsRepo.h"
#include "db/Db.h"
#include <sqlite3.h>
#include <stdexcept>
#include <string>
#include <optional>

// безпечно читаємо TEXT (NULL -> "")
static std::string textOrEmpty(sqlite3_stmt* st, int col) {
    const unsigned char* p = sqlite3_column_text(st, col);
    return p ? reinterpret_cast<const char*>(p) : std::string();
}

// допоміжне: зчитати рядок в Ship (з урахуванням company_id = 7-й стовпець)
static Ship parseShip(sqlite3_stmt* st) {
    Ship s;
    s.id         = sqlite3_column_int64(st, 0);
    s.name       = textOrEmpty(st, 1);
    s.type       = textOrEmpty(st, 2);
    s.country    = textOrEmpty(st, 3);
    s.port_id    = sqlite3_column_int64(st, 4);
    s.status     = textOrEmpty(st, 5);
    s.company_id = (sqlite3_column_type(st, 6) == SQLITE_NULL) ? 0 : sqlite3_column_int64(st, 6);
    return s;
}

// допоміжне: підібрати валідний port_id
static long long resolvePortId(sqlite3* db, long long desired) {
    // якщо заданий і існує — лишаємо
    if (desired > 0) {
        sqlite3_stmt* st{};
        if (sqlite3_prepare_v2(db, "SELECT 1 FROM ports WHERE id=? LIMIT 1", -1, &st, nullptr) == SQLITE_OK) {
            sqlite3_bind_int64(st, 1, desired);
            int rc = sqlite3_step(st);
            sqlite3_finalize(st);
            if (rc == SQLITE_ROW) return desired;
        }
        // інакше впадемо на підбір першого порту
    }
    // підбираємо перший доступний порт
    sqlite3_stmt* st{};
    if (sqlite3_prepare_v2(db, "SELECT id FROM ports ORDER BY id LIMIT 1", -1, &st, nullptr) != SQLITE_OK) {
        throw std::runtime_error(std::string("resolvePortId prepare failed: ") + sqlite3_errmsg(db));
    }
    long long id = 0;
    if (sqlite3_step(st) == SQLITE_ROW) id = sqlite3_column_int64(st, 0);
    sqlite3_finalize(st);
    if (id == 0) throw std::runtime_error("No ports available; seed ports first");
    return id;
}

std::vector<Ship> ShipsRepo::all() {
    std::vector<Ship> out;
    sqlite3* db = Db::instance().handle();
    const char* sql = "SELECT id,name,type,country,port_id,status,company_id FROM ships ORDER BY id";
    sqlite3_stmt* st{};
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) return out;
    while (sqlite3_step(st) == SQLITE_ROW) out.push_back(parseShip(st));
    sqlite3_finalize(st);
    return out;
}

std::vector<Ship> ShipsRepo::getByPortId(long long portId) {
    std::vector<Ship> out;
    sqlite3* db = Db::instance().handle();
    const char* sql = "SELECT id,name,type,country,port_id,status,company_id FROM ships WHERE port_id=? ORDER BY id";
    sqlite3_stmt* st{};
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) return out;
    sqlite3_bind_int64(st, 1, portId);
    while (sqlite3_step(st) == SQLITE_ROW) out.push_back(parseShip(st));
    sqlite3_finalize(st);
    return out;
}

Ship ShipsRepo::create(const Ship& s) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "INSERT INTO ships(name,type,country,port_id,status,company_id) VALUES(?,?,?,?,?,?)";
    sqlite3_stmt* st{};
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) {
        throw std::runtime_error(std::string("prepare failed: ") + sqlite3_errmsg(db));
    }

    const std::string type    = s.type.empty()    ? "Cargo"   : s.type;
    const std::string country = s.country.empty() ? "Unknown" : s.country;
    const std::string status  = s.status.empty()  ? "docked"  : s.status;
    const long long   portId  = resolvePortId(db, s.port_id);

    sqlite3_bind_text (st, 1, s.name.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (st, 2, type.c_str(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (st, 3, country.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(st, 4, portId);
    sqlite3_bind_text (st, 5, status.c_str(),  -1, SQLITE_TRANSIENT);
    if (s.company_id > 0) sqlite3_bind_int64(st, 6, s.company_id);
    else                  sqlite3_bind_null  (st, 6);

    if (sqlite3_step(st) != SQLITE_DONE) {
        std::string emsg = sqlite3_errmsg(db);
        sqlite3_finalize(st);
        throw std::runtime_error("insert failed: " + emsg);
    }
    sqlite3_finalize(st);

    long long id = sqlite3_last_insert_rowid(db);
    auto got = byId(id);
    if (!got) throw std::runtime_error("insert ok but fetch failed");
    return *got;
}

std::optional<Ship> ShipsRepo::byId(long long id) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "SELECT id,name,type,country,port_id,status,company_id FROM ships WHERE id=?";
    sqlite3_stmt* st{};
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) return std::nullopt;
    sqlite3_bind_int64(st, 1, id);
    std::optional<Ship> out;
    if (sqlite3_step(st) == SQLITE_ROW) out = parseShip(st);
    sqlite3_finalize(st);
    return out;
}

void ShipsRepo::update(const Ship& s) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "UPDATE ships SET name=?, type=?, country=?, port_id=?, status=?, company_id=? WHERE id=?";
    sqlite3_stmt* st{};
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) {
        throw std::runtime_error(std::string("prepare failed: ") + sqlite3_errmsg(db));
    }

    sqlite3_bind_text (st, 1, s.name.c_str(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (st, 2, s.type.c_str(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (st, 3, s.country.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(st, 4, s.port_id);
    sqlite3_bind_text (st, 5, s.status.c_str(),  -1, SQLITE_TRANSIENT);
    if (s.company_id > 0) sqlite3_bind_int64(st, 6, s.company_id);
    else                  sqlite3_bind_null  (st, 6);
    sqlite3_bind_int64(st, 7, s.id);

    if (sqlite3_step(st) != SQLITE_DONE) {
        std::string emsg = sqlite3_errmsg(db);
        sqlite3_finalize(st);
        throw std::runtime_error("update failed: " + emsg);
    }
    sqlite3_finalize(st);
}

void ShipsRepo::remove(long long id) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "DELETE FROM ships WHERE id=?";
    sqlite3_stmt* st{};
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) {
        throw std::runtime_error(std::string("prepare failed: ") + sqlite3_errmsg(db));
    }
    sqlite3_bind_int64(st, 1, id);
    if (sqlite3_step(st) != SQLITE_DONE) {
        std::string emsg = sqlite3_errmsg(db);
        sqlite3_finalize(st);
        throw std::runtime_error("delete failed: " + emsg);
    }
    sqlite3_finalize(st);
}
