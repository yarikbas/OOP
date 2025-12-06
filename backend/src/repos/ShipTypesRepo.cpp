#include "repos/ShipTypesRepo.h"
#include "db/Db.h"

#include <sqlite3.h>

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

// ---------- RAII wrapper for sqlite3_stmt ----------
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

ShipType parseType(sqlite3_stmt* st) {
    ShipType t{};
    t.id          = sqlite3_column_int64(st, 0);
    t.code        = safe_text(st, 1);
    t.name        = safe_text(st, 2);
    t.description = safe_text(st, 3);
    return t;
}

} // namespace

std::vector<ShipType> ShipTypesRepo::all() {
    std::vector<ShipType> out;
    sqlite3* db = Db::instance().handle();

    const char* sql =
        "SELECT id, code, name, description "
        "FROM ship_types "
        "ORDER BY id";

    Stmt st(db, sql);

    while (sqlite3_step(st.get()) == SQLITE_ROW) {
        out.push_back(parseType(st.get()));
    }

    return out;
}

std::optional<ShipType> ShipTypesRepo::byId(long long id) {
    sqlite3* db = Db::instance().handle();

    const char* sql =
        "SELECT id, code, name, description "
        "FROM ship_types "
        "WHERE id=?";

    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, static_cast<std::int64_t>(id));

    if (sqlite3_step(st.get()) == SQLITE_ROW) {
        return parseType(st.get());
    }

    return std::nullopt;
}

std::optional<ShipType> ShipTypesRepo::byCode(const std::string& code) {
    sqlite3* db = Db::instance().handle();

    const char* sql =
        "SELECT id, code, name, description "
        "FROM ship_types "
        "WHERE code=?";

    Stmt st(db, sql);
    sqlite3_bind_text(st.get(), 1, code.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(st.get()) == SQLITE_ROW) {
        return parseType(st.get());
    }

    return std::nullopt;
}

ShipType ShipTypesRepo::create(const ShipType& t) {
    sqlite3* db = Db::instance().handle();

    const char* sql =
        "INSERT INTO ship_types(code,name,description) "
        "VALUES(?,?,?)";

    Stmt st(db, sql);

    sqlite3_bind_text(st.get(), 1, t.code.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.get(), 2, t.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.get(), 3, t.description.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(st.get()) != SQLITE_DONE) {
        throw std::runtime_error(std::string("insert failed: ") + sqlite3_errmsg(db));
    }

    const auto id = sqlite3_last_insert_rowid(db);
    auto got = byId(id);
    if (!got) throw std::runtime_error("insert ok but fetch failed");
    return *got;
}

void ShipTypesRepo::update(const ShipType& t) {
    sqlite3* db = Db::instance().handle();

    const char* sql =
        "UPDATE ship_types "
        "SET code=?, name=?, description=? "
        "WHERE id=?";

    Stmt st(db, sql);

    sqlite3_bind_text(st.get(), 1, t.code.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.get(), 2, t.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.get(), 3, t.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(st.get(), 4, static_cast<std::int64_t>(t.id));

    if (sqlite3_step(st.get()) != SQLITE_DONE) {
        throw std::runtime_error(std::string("update failed: ") + sqlite3_errmsg(db));
    }
}

void ShipTypesRepo::remove(long long id) {
    sqlite3* db = Db::instance().handle();

    const char* sql =
        "DELETE FROM ship_types "
        "WHERE id=?";

    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, static_cast<std::int64_t>(id));

    if (sqlite3_step(st.get()) != SQLITE_DONE) {
        throw std::runtime_error(std::string("delete failed: ") + sqlite3_errmsg(db));
    }
}
