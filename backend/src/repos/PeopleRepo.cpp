#include "repos/PeopleRepo.h"
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

Person parsePerson(sqlite3_stmt* st) {
    Person p{};
    p.id        = sqlite3_column_int64(st, 0);
    p.full_name = safe_text(st, 1);
    p.rank      = safe_text(st, 2);
    p.active    = sqlite3_column_int(st, 3);
    return p;
}

} // namespace

std::vector<Person> PeopleRepo::all() {
    std::vector<Person> out;
    sqlite3* db = Db::instance().handle();

    const char* sql =
        "SELECT id, full_name, rank, active "
        "FROM people "
        "ORDER BY id";

    Stmt st(db, sql);

    while (sqlite3_step(st.get()) == SQLITE_ROW) {
        out.push_back(parsePerson(st.get()));
    }

    return out;
}

std::optional<Person> PeopleRepo::byId(long long id) {
    sqlite3* db = Db::instance().handle();

    const char* sql =
        "SELECT id, full_name, rank, active "
        "FROM people "
        "WHERE id=?";

    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, static_cast<std::int64_t>(id));

    if (sqlite3_step(st.get()) == SQLITE_ROW) {
        return parsePerson(st.get());
    }

    return std::nullopt;
}

Person PeopleRepo::create(const Person& p) {
    sqlite3* db = Db::instance().handle();

    const char* sql =
        "INSERT INTO people(full_name, rank, active) "
        "VALUES (?,?,?)";

    Stmt st(db, sql);

    sqlite3_bind_text(st.get(), 1, p.full_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.get(), 2, p.rank.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (st.get(), 3, p.active);

    if (sqlite3_step(st.get()) != SQLITE_DONE) {
        throw std::runtime_error(std::string("insert failed: ") + sqlite3_errmsg(db));
    }

    const auto newId = sqlite3_last_insert_rowid(db);
    auto got = byId(newId);
    if (!got) throw std::runtime_error("insert ok but fetch failed");
    return *got;
}

void PeopleRepo::update(const Person& p) {
    sqlite3* db = Db::instance().handle();

    const char* sql =
        "UPDATE people "
        "SET full_name=?, rank=?, active=? "
        "WHERE id=?";

    Stmt st(db, sql);

    sqlite3_bind_text (st.get(), 1, p.full_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text (st.get(), 2, p.rank.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int  (st.get(), 3, p.active);
    sqlite3_bind_int64(st.get(), 4, static_cast<std::int64_t>(p.id));

    if (sqlite3_step(st.get()) != SQLITE_DONE) {
        throw std::runtime_error(std::string("update failed: ") + sqlite3_errmsg(db));
    }
}

void PeopleRepo::remove(long long id) {
    sqlite3* db = Db::instance().handle();

    const char* sql =
        "DELETE FROM people "
        "WHERE id=?";

    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, static_cast<std::int64_t>(id));

    if (sqlite3_step(st.get()) != SQLITE_DONE) {
        throw std::runtime_error(std::string("delete failed: ") + sqlite3_errmsg(db));
    }

    // Семантику "void remove" залишаємо як є:
    // якщо id не існує — просто 0 changes без винятку.
}
