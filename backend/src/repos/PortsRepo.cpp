#include "repos/PortsRepo.h"
#include "db/Db.h"

#include <sqlite3.h>

#include <stdexcept>
#include <string>
#include <vector>
#include <optional>
#include <cstdint>

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

Port parsePort(sqlite3_stmt* st) {
    Port p{};
    p.id     = sqlite3_column_int64(st, 0);
    p.name   = safe_text(st, 1);
    p.region = safe_text(st, 2);
    p.lat    = sqlite3_column_double(st, 3);
    p.lon    = sqlite3_column_double(st, 4);
    return p;
}

} // namespace

PortsRepo::PortsRepo()
    : db_(Db::instance().handle()) {}

PortsRepo::PortsRepo(sqlite3* db)
    : db_(db) {}

// ------------------ READ ALL ------------------

std::vector<Port> PortsRepo::all() const {
    std::vector<Port> result;

    const char* sql =
        "SELECT id, name, region, lat, lon "
        "FROM ports "
        "ORDER BY id;";

    Stmt st(db_, sql);

    while (sqlite3_step(st.get()) == SQLITE_ROW) {
        result.push_back(parsePort(st.get()));
    }

    return result;
}

// ------------------ CREATE ------------------

Port PortsRepo::create(const Port& in) const {
    const char* sql =
        "INSERT INTO ports (name, region, lat, lon) "
        "VALUES (?, ?, ?, ?);";

    Stmt st(db_, sql);

    sqlite3_bind_text  (st.get(), 1, in.name.c_str(),   -1, SQLITE_TRANSIENT);
    sqlite3_bind_text  (st.get(), 2, in.region.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(st.get(), 3, in.lat);
    sqlite3_bind_double(st.get(), 4, in.lon);

    const int rc = sqlite3_step(st.get());
    if (rc != SQLITE_DONE) {
        throw std::runtime_error(std::string("PortsRepo::create failed: ") + sqlite3_errmsg(db_));
    }

    Port out = in;
    out.id = sqlite3_last_insert_rowid(db_);
    return out;
}

// ------------------ GET BY ID ------------------

std::optional<Port> PortsRepo::getById(int64_t id) const {
    const char* sql =
        "SELECT id, name, region, lat, lon "
        "FROM ports "
        "WHERE id = ?;";

    Stmt st(db_, sql);
    sqlite3_bind_int64(st.get(), 1, id);

    const int rc = sqlite3_step(st.get());
    if (rc == SQLITE_ROW) {
        return parsePort(st.get());
    }

    return std::nullopt;
}

// ------------------ UPDATE ------------------

bool PortsRepo::update(const Port& p) const {
    const char* sql =
        "UPDATE ports "
        "SET name = ?, region = ?, lat = ?, lon = ? "
        "WHERE id = ?;";

    Stmt st(db_, sql);

    sqlite3_bind_text  (st.get(), 1, p.name.c_str(),   -1, SQLITE_TRANSIENT);
    sqlite3_bind_text  (st.get(), 2, p.region.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(st.get(), 3, p.lat);
    sqlite3_bind_double(st.get(), 4, p.lon);
    sqlite3_bind_int64 (st.get(), 5, p.id);

    const int rc = sqlite3_step(st.get());
    if (rc != SQLITE_DONE) {
        return false;
    }

    return sqlite3_changes(db_) > 0;
}

// ------------------ REMOVE ------------------

bool PortsRepo::remove(int64_t id) const {
    const char* sql =
        "DELETE FROM ports "
        "WHERE id = ?;";

    Stmt st(db_, sql);
    sqlite3_bind_int64(st.get(), 1, id);

    const int rc = sqlite3_step(st.get());
    if (rc != SQLITE_DONE) {
        return false;
    }

    return sqlite3_changes(db_) > 0;
}
