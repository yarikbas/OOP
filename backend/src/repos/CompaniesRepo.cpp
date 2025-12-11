// src/repos/CompaniesRepo.cpp
#include "repos/CompaniesRepo.h"
#include "db/Db.h"

#include <sqlite3.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace {

// ---- RAII stmt ---------------------------------

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

// ---- helpers -----------------------------------

inline std::string safe_text(sqlite3_stmt* st, int col) {
    const char* ptr = reinterpret_cast<const char*>(sqlite3_column_text(st, col));
    return ptr ? ptr : "";
}

Company parseCompany(sqlite3_stmt* st) {
    Company c{};
    c.id   = sqlite3_column_int64(st, 0);
    c.name = safe_text(st, 1);
    return c;
}

Port parsePort(sqlite3_stmt* st) {
    Port p{};
    p.id     = sqlite3_column_int64(st, 0);
    p.name   = safe_text(st, 1);
    p.region = safe_text(st, 2);
    p.lat    = sqlite3_column_double(st, 3);
    p.lon    = sqlite3_column_double(st, 4);
    return p;
}

Ship parseShip(sqlite3_stmt* st) {
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

void execSimple(sqlite3* db, const char* sql) {
    char* err = nullptr;
    const int rc = sqlite3_exec(db, sql, nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::string msg = err ? err : sqlite3_errmsg(db);
        if (err) sqlite3_free(err);
        throw std::runtime_error(msg);
    }
}

} // namespace

// ---- CRUD companies --------------------------------------------

std::vector<Company> CompaniesRepo::all() {
    std::vector<Company> out;
    sqlite3* db = Db::instance().handle();

    const char* sql = "SELECT id,name FROM companies ORDER BY id";

    Stmt st(db, sql);
    while (sqlite3_step(st.get()) == SQLITE_ROW) {
        out.push_back(parseCompany(st.get()));
    }
    return out;
}

std::optional<Company> CompaniesRepo::byId(std::int64_t id) {
    sqlite3* db = Db::instance().handle();

    const char* sql = "SELECT id,name FROM companies WHERE id=?";

    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, id);

    if (sqlite3_step(st.get()) == SQLITE_ROW) {
        return parseCompany(st.get());
    }
    return std::nullopt;
}

Company CompaniesRepo::create(const std::string& name) {
    sqlite3* db = Db::instance().handle();

    const char* sql = "INSERT INTO companies(name) VALUES(?)";

    Stmt st(db, sql);
    sqlite3_bind_text(st.get(), 1, name.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(st.get()) != SQLITE_DONE) {
        throw std::runtime_error(std::string("insert company failed: ") + sqlite3_errmsg(db));
    }

    const std::int64_t id = sqlite3_last_insert_rowid(db);
    auto c = byId(id);
    if (!c) throw std::runtime_error("insert ok but fetch failed");

    try {
        std::string msg = "Created company '" + name + "' (id=" + std::to_string(id) + ")";
        Db::instance().insertLog("AUDIT", "company.create", "company", (int)id, "system", msg);
    } catch (...) {}
    return *c;
}

// overload під тести
Company CompaniesRepo::create(const Company& c) {
    return create(c.name);
}

bool CompaniesRepo::update(std::int64_t id, const std::string& name) {
    sqlite3* db = Db::instance().handle();

    const char* sql = "UPDATE companies SET name=? WHERE id=?";

    Stmt st(db, sql);
    sqlite3_bind_text(st.get(), 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(st.get(), 2, id);

    if (sqlite3_step(st.get()) != SQLITE_DONE) {
        throw std::runtime_error(std::string("update company failed: ") + sqlite3_errmsg(db));
    }

    const bool changed = sqlite3_changes(db) > 0;
    if (changed) {
        try {
            std::string msg = "Updated company id=" + std::to_string(id) + " name='" + name + "'";
            Db::instance().insertLog("AUDIT", "company.update", "company", (int)id, "system", msg);
        } catch (...) {}
    }

    return changed;
}

// ✅ overload під тести
bool CompaniesRepo::update(const Company& c) {
    return update(c.id, c.name);
}

bool CompaniesRepo::remove(std::int64_t id) {
    sqlite3* db = Db::instance().handle();

    const char* sql = "DELETE FROM companies WHERE id=?";

    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, id);

    if (sqlite3_step(st.get()) != SQLITE_DONE) {
        throw std::runtime_error(std::string("delete company failed: ") + sqlite3_errmsg(db));
    }

    const bool changed = sqlite3_changes(db) > 0;
    if (changed) {
        try {
            std::string msg = "Deleted company id=" + std::to_string(id);
            Db::instance().insertLog("AUDIT", "company.delete", "company", (int)id, "system", msg);
        } catch (...) {}
    }

    return changed;
}

// ---- company ↔ ports -------------------------------------------

std::vector<Port> CompaniesRepo::ports(std::int64_t companyId) {
    std::vector<Port> out;
    sqlite3* db = Db::instance().handle();

    const char* sql =
        "SELECT p.id,p.name,p.region,p.lat,p.lon "
        "FROM company_ports cp "
        "JOIN ports p ON p.id = cp.port_id "
        "WHERE cp.company_id=? "
        "ORDER BY p.id";

    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, companyId);

    while (sqlite3_step(st.get()) == SQLITE_ROW) {
        out.push_back(parsePort(st.get()));
    }
    return out;
}

bool CompaniesRepo::addPort(std::int64_t companyId, std::int64_t portId, bool isMain) {
    sqlite3* db = Db::instance().handle();

    if (isMain) {
        execSimple(db, "BEGIN;");
    }

    try {
        if (isMain) {
            const char* clearSql =
                "UPDATE company_ports SET is_main=0 WHERE company_id=?";

            Stmt clear(db, clearSql);
            sqlite3_bind_int64(clear.get(), 1, companyId);

            if (sqlite3_step(clear.get()) != SQLITE_DONE) {
                throw std::runtime_error(std::string("clear main port failed: ") + sqlite3_errmsg(db));
            }
        }

        // Upsert
        const char* upsertSql =
            "INSERT INTO company_ports(company_id,port_id,is_main) "
            "VALUES(?,?,?) "
            "ON CONFLICT(company_id,port_id) "
            "DO UPDATE SET is_main=excluded.is_main;";

        Stmt up(db, upsertSql);
        sqlite3_bind_int64(up.get(), 1, companyId);
        sqlite3_bind_int64(up.get(), 2, portId);
        sqlite3_bind_int64(up.get(), 3, isMain ? 1 : 0);

        if (sqlite3_step(up.get()) != SQLITE_DONE) {
            throw std::runtime_error(std::string("add port failed: ") + sqlite3_errmsg(db));
        }

        if (isMain) {
            execSimple(db, "COMMIT;");
        }
        try {
            std::string msg = "Added port_id=" + std::to_string(portId) + " to company_id=" + std::to_string(companyId) + (isMain ? " (main)" : "");
            Db::instance().insertLog("AUDIT", "company.add_port", "company", (int)companyId, "system", msg);
        } catch (...) {}
        return true;
    } catch (...) {
        if (isMain) {
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        }
        throw;
    }
}

bool CompaniesRepo::removePort(std::int64_t companyId, std::int64_t portId) {
    sqlite3* db = Db::instance().handle();

    const char* sql =
        "DELETE FROM company_ports WHERE company_id=? AND port_id=?";

    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, companyId);
    sqlite3_bind_int64(st.get(), 2, portId);

    if (sqlite3_step(st.get()) != SQLITE_DONE) {
        throw std::runtime_error(std::string("remove port failed: ") + sqlite3_errmsg(db));
    }
    const bool changed = sqlite3_changes(db) > 0;
    if (changed) {
        try {
            std::string msg = "Removed port_id=" + std::to_string(portId) + " from company_id=" + std::to_string(companyId);
            Db::instance().insertLog("AUDIT", "company.remove_port", "company", (int)companyId, "system", msg);
        } catch (...) {}
    }
    return changed;
}

// ---- company ↔ ships -------------------------------------------

std::vector<Ship> CompaniesRepo::ships(std::int64_t companyId) {
    std::vector<Ship> out;
    sqlite3* db = Db::instance().handle();

    const char* sql =
        "SELECT id,name,type,country,port_id,status,IFNULL(company_id,0) "
        "FROM ships WHERE company_id=? ORDER BY id";

    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, companyId);

    while (sqlite3_step(st.get()) == SQLITE_ROW) {
        out.push_back(parseShip(st.get()));
    }
    return out;
}
