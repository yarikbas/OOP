#include "repos/CrewRepo.h"
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

CrewAssignment parseRow(sqlite3_stmt* st) {
    CrewAssignment a{};
    a.id        = sqlite3_column_int64(st, 0);
    a.person_id = sqlite3_column_int64(st, 1);
    a.ship_id   = sqlite3_column_int64(st, 2);
    a.start_utc = safe_text(st, 3);

    const auto end = safe_text(st, 4);
    if (!end.empty()) {
        a.end_utc = end;
    }
    return a;
}

bool isConstraintError(int rc) {
    return rc == SQLITE_CONSTRAINT ||
           rc == SQLITE_CONSTRAINT_UNIQUE ||
           rc == SQLITE_CONSTRAINT_PRIMARYKEY;
}

} // namespace

std::vector<CrewAssignment> CrewRepo::currentCrewByShip(long long shipId) {
    std::vector<CrewAssignment> out;
    sqlite3* db = Db::instance().handle();

    const char* sql =
        "SELECT id, person_id, ship_id, start_utc, end_utc "
        "FROM crew_assignments "
        "WHERE ship_id=? AND end_utc IS NULL "
        "ORDER BY id";

    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, static_cast<std::int64_t>(shipId));

    while (sqlite3_step(st.get()) == SQLITE_ROW) {
        out.push_back(parseRow(st.get()));
    }

    return out;
}

std::optional<CrewAssignment> CrewRepo::assign(long long personId,
                                               long long shipId,
                                               const std::string& startUtc) {
    sqlite3* db = Db::instance().handle();

    // Покладаємось на partial unique index:
    // ux_crew_person_active(person_id) WHERE end_utc IS NULL
    const char* insSql =
        "INSERT INTO crew_assignments(person_id, ship_id, start_utc) "
        "VALUES(?,?,?)";

    try {
        Stmt ins(db, insSql);

        sqlite3_bind_int64(ins.get(), 1, static_cast<std::int64_t>(personId));
        sqlite3_bind_int64(ins.get(), 2, static_cast<std::int64_t>(shipId));
        sqlite3_bind_text (ins.get(), 3, startUtc.c_str(), -1, SQLITE_TRANSIENT);

        const int rc = sqlite3_step(ins.get());
        if (rc != SQLITE_DONE) {
            // Конфлікт активного призначення → бізнес-відмова
            if (isConstraintError(rc)) {
                return std::nullopt;
            }
            throw std::runtime_error(sqlite3_errmsg(db));
        }

        const std::int64_t id = sqlite3_last_insert_rowid(db);

        const char* selSql =
            "SELECT id, person_id, ship_id, start_utc, end_utc "
            "FROM crew_assignments WHERE id=?";

        Stmt sel(db, selSql);
        sqlite3_bind_int64(sel.get(), 1, id);

        if (sqlite3_step(sel.get()) == SQLITE_ROW) {
            return parseRow(sel.get());
        }

        throw std::runtime_error("insert ok but fetch failed");
    } catch (const std::runtime_error&) {
        // даємо контролеру вирішити 500
        throw;
    }
}

bool CrewRepo::endActiveByPerson(long long personId, const std::string& endUtc) {
    sqlite3* db = Db::instance().handle();

    const char* sql =
        "UPDATE crew_assignments "
        "SET end_utc=? "
        "WHERE person_id=? AND end_utc IS NULL";

    try {
        Stmt st(db, sql);

        sqlite3_bind_text (st.get(), 1, endUtc.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(st.get(), 2, static_cast<std::int64_t>(personId));

        const int rc = sqlite3_step(st.get());
        if (rc != SQLITE_DONE) {
            return false;
        }

        return sqlite3_changes(db) > 0;
    } catch (...) {
        return false;
    }
}
