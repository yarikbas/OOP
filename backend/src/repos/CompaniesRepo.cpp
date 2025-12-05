#include "repos/CompaniesRepo.h"
#include "db/Db.h"
#include "models/Company.h"
#include "models/Port.h"
#include "models/Ship.h"
#include <sqlite3.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <optional>

// ---- helpers (ВИПРАВЛЕНО: Безпечна обробка NULL) ------------------

static inline std::string safe_text(sqlite3_stmt* st, int col) {
    const char* ptr = reinterpret_cast<const char*>(sqlite3_column_text(st, col));
    return (ptr ? ptr : "");
}

static Company parseCompany(sqlite3_stmt* st) {
    Company c{};
    c.id   = sqlite3_column_int64(st, 0);
    c.name = safe_text(st, 1);
    return c;
}

static Port parsePort(sqlite3_stmt* st) {
    Port p{};
    p.id     = sqlite3_column_int64(st, 0);
    p.name   = safe_text(st, 1);
    p.region = safe_text(st, 2);
    p.lat    = sqlite3_column_double(st, 3);
    p.lon    = sqlite3_column_double(st, 4);
    return p;
}

static Ship parseShip(sqlite3_stmt* st) {
    Ship s{};
    s.id         = sqlite3_column_int64(st, 0);
    s.name       = safe_text(st, 1);
    s.type       = safe_text(st, 2);
    s.country    = safe_text(st, 3);
    s.port_id    = sqlite3_column_int64(st, 4);
    s.status     = safe_text(st, 5);
    s.company_id = sqlite3_column_int64(st, 6);
    return s;
}

// ---- CRUD companies --------------------------------------------
std::vector<Company> CompaniesRepo::all() {
    std::vector<Company> out;
    sqlite3* db = Db::instance().handle();
    const char* sql = "SELECT id,name FROM companies ORDER BY id";
    sqlite3_stmt* st{};
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) return out;
    while (sqlite3_step(st) == SQLITE_ROW) out.push_back(parseCompany(st));
    sqlite3_finalize(st);
    return out;
}

std::optional<Company> CompaniesRepo::byId(long long id) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "SELECT id,name FROM companies WHERE id=?";
    sqlite3_stmt* st{};
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) return std::nullopt;
    sqlite3_bind_int64(st, 1, id);
    std::optional<Company> out;
    if (sqlite3_step(st) == SQLITE_ROW) out = parseCompany(st);
    sqlite3_finalize(st);
    return out;
}

Company CompaniesRepo::create(const std::string& name) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "INSERT INTO companies(name) VALUES(?)";
    sqlite3_stmt* st{};
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) {
        throw std::runtime_error(std::string("prepare failed: ") + sqlite3_errmsg(db));
    }
    sqlite3_bind_text(st, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(st) != SQLITE_DONE) {
        std::string em = sqlite3_errmsg(db);
        sqlite3_finalize(st);
        throw std::runtime_error("insert company failed: " + em);
    }
    sqlite3_finalize(st);

    long long id = sqlite3_last_insert_rowid(db);
    auto c = byId(id);
    if (!c) throw std::runtime_error("insert ok but fetch failed");
    return *c;
}

bool CompaniesRepo::update(long long id, const std::string& name) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "UPDATE companies SET name=? WHERE id=?";
    sqlite3_stmt* st{};
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text (st, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(st, 2, id);
    bool ok = (sqlite3_step(st) == SQLITE_DONE) && (sqlite3_changes(db) > 0);
    sqlite3_finalize(st);
    return ok;
}

bool CompaniesRepo::remove(long long id) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "DELETE FROM companies WHERE id=?";
    sqlite3_stmt* st{};
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int64(st, 1, id);
    bool ok = (sqlite3_step(st) == SQLITE_DONE) && (sqlite3_changes(db) > 0);
    sqlite3_finalize(st);
    return ok;
}

// ---- company ↔ ports -------------------------------------------
std::vector<Port> CompaniesRepo::ports(long long companyId) {
    std::vector<Port> out;
    sqlite3* db = Db::instance().handle();
    const char* sql =
        "SELECT p.id,p.name,p.region,p.lat,p.lon "
        "FROM company_ports cp "
        "JOIN ports p ON p.id = cp.port_id "
        "WHERE cp.company_id=? "
        "ORDER BY p.id";
    sqlite3_stmt* st{};
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) return out;
    sqlite3_bind_int64(st, 1, companyId);
    while (sqlite3_step(st) == SQLITE_ROW) out.push_back(parsePort(st));
    sqlite3_finalize(st);
    return out;
}

bool CompaniesRepo::addPort(long long companyId, long long portId, bool main) {
    sqlite3* db = Db::instance().handle();

    // якщо main=true — скидаємо інші "головні" для цієї компанії
    if (main) {
        const char* clearMainSql = "UPDATE company_ports SET is_main=0 WHERE company_id=?";
        sqlite3_stmt* clear{};
        if (sqlite3_prepare_v2(db, clearMainSql, -1, &clear, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_int64(clear, 1, companyId);
        if (sqlite3_step(clear) != SQLITE_DONE) { sqlite3_finalize(clear); return false; }
        sqlite3_finalize(clear);
    }

    // Перший крок: пробуємо INSERT
    const char* insSql =
        "INSERT INTO company_ports(company_id,port_id,is_main) VALUES(?,?,?)";
    sqlite3_stmt* ins{};
    if (sqlite3_prepare_v2(db, insSql, -1, &ins, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int64(ins, 1, companyId);
    sqlite3_bind_int64(ins, 2, portId);
    sqlite3_bind_int64(ins, 3, main ? 1 : 0);

    int rc = sqlite3_step(ins);
    sqlite3_finalize(ins);

    if (rc == SQLITE_DONE) {
        // Успішна вставка
        return true;
    }

    // Якщо конфлікт (рядок вже існує) — робимо UPDATE is_main
    if (rc == SQLITE_CONSTRAINT || rc == SQLITE_CONSTRAINT_PRIMARYKEY || rc == SQLITE_CONSTRAINT_UNIQUE) {
        const char* updSql =
            "UPDATE company_ports SET is_main=? WHERE company_id=? AND port_id=?";
        sqlite3_stmt* upd{};
        if (sqlite3_prepare_v2(db, updSql, -1, &upd, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_int64(upd, 1, main ? 1 : 0);
        sqlite3_bind_int64(upd, 2, companyId);
        sqlite3_bind_int64(upd, 3, portId);
        int urc = sqlite3_step(upd);
        sqlite3_finalize(upd);
        // Вважаємо успіхом: або щось оновили, або значення і так було таким самим
        return (urc == SQLITE_DONE);
    }

    // Інша помилка
    return false;
}


bool CompaniesRepo::removePort(long long companyId, long long portId) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "DELETE FROM company_ports WHERE company_id=? AND port_id=?";
    sqlite3_stmt* st{};
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int64(st, 1, companyId);
    sqlite3_bind_int64(st, 2, portId);
    bool ok = (sqlite3_step(st) == SQLITE_DONE) && (sqlite3_changes(db) > 0);
    sqlite3_finalize(st);
    return ok;
}

// ---- company ↔ ships -------------------------------------------
std::vector<Ship> CompaniesRepo::ships(long long companyId) {
    std::vector<Ship> out;
    sqlite3* db = Db::instance().handle();
    const char* sql =
        "SELECT id,name,type,country,port_id,status,IFNULL(company_id,0) "
        "FROM ships WHERE company_id=? ORDER BY id";
    sqlite3_stmt* st{};
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) return out;
    sqlite3_bind_int64(st, 1, companyId);
    while (sqlite3_step(st) == SQLITE_ROW) out.push_back(parseShip(st));
    sqlite3_finalize(st);
    return out;
}