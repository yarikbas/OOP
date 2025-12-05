#include "repos/PeopleRepo.h"
#include "db/Db.h"
#include <sqlite3.h>
#include <stdexcept>

static Person parse(sqlite3_stmt* st) {
    Person p;
    p.id = sqlite3_column_int64(st, 0);
    const unsigned char* t1 = sqlite3_column_text(st, 1);
    p.full_name = t1 ? reinterpret_cast<const char*>(t1) : "";
    const unsigned char* t2 = sqlite3_column_text(st, 2);
    p.rank = t2 ? reinterpret_cast<const char*>(t2) : "";
    p.active = sqlite3_column_int(st, 3);
    return p;
}

std::vector<Person> PeopleRepo::all() {
    std::vector<Person> out;
    sqlite3* db = Db::instance().handle();
    const char* sql = "SELECT id, full_name, rank, active FROM people ORDER BY id";
    sqlite3_stmt* st{};
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) return out;
    while (sqlite3_step(st) == SQLITE_ROW) out.push_back(parse(st));
    sqlite3_finalize(st);
    return out;
}

std::optional<Person> PeopleRepo::byId(long long id) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "SELECT id, full_name, rank, active FROM people WHERE id=?";
    sqlite3_stmt* st{};
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) return std::nullopt;
    sqlite3_bind_int64(st, 1, id);
    std::optional<Person> out;
    if (sqlite3_step(st) == SQLITE_ROW) out = parse(st);
    sqlite3_finalize(st);
    return out;
}

Person PeopleRepo::create(const Person& p) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "INSERT INTO people(full_name, rank, active) VALUES (?,?,?)";
    sqlite3_stmt* st{};
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK)
        throw std::runtime_error(std::string("prepare failed: ") + sqlite3_errmsg(db));
    sqlite3_bind_text(st, 1, p.full_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 2, p.rank.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(st, 3, p.active);
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

void PeopleRepo::update(const Person& p) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "UPDATE people SET full_name=?, rank=?, active=? WHERE id=?";
    sqlite3_stmt* st{};
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK)
        throw std::runtime_error(std::string("prepare failed: ") + sqlite3_errmsg(db));
    sqlite3_bind_text(st, 1, p.full_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 2, p.rank.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(st, 3, p.active);
    sqlite3_bind_int64(st, 4, p.id);
    if (sqlite3_step(st) != SQLITE_DONE) {
        std::string e = sqlite3_errmsg(db);
        sqlite3_finalize(st);
        throw std::runtime_error("update failed: " + e);
    }
    sqlite3_finalize(st);
}

void PeopleRepo::remove(long long id) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "DELETE FROM people WHERE id=?";
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
