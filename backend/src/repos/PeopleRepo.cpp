#include "repos/PeopleRepo.h"
#include "db/Db.h"
#include <sqlite3.h>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
    // Допоміжна функція для безпечного читання тексту
    std::string safe_text(sqlite3_stmt* st, int col) {
        const unsigned char* t = sqlite3_column_text(st, col);
        return t ? reinterpret_cast<const char*>(t) : "";
    }
}

void PeopleRepo::createTable() {
    sqlite3* db = Db::instance().handle();
    const char* sql =
        "CREATE TABLE IF NOT EXISTS people ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  full_name TEXT NOT NULL,"
        "  rank TEXT NOT NULL"
        ");";

    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::string msg = errMsg;
        sqlite3_free(errMsg);
        throw std::runtime_error("PeopleRepo::createTable failed: " + msg);
    }
}

Person PeopleRepo::create(const Person& p) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "INSERT INTO people(full_name, rank) VALUES(?, ?);";

    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }

    // Прив'язуємо параметри
    sqlite3_bind_text(st, 1, p.full_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 2, p.rank.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(st) != SQLITE_DONE) {
        sqlite3_finalize(st);
        throw std::runtime_error("PeopleRepo::create step failed: " + std::string(sqlite3_errmsg(db)));
    }
    sqlite3_finalize(st);

    Person created = p;
    created.id = sqlite3_last_insert_rowid(db);
    return created;
}

std::vector<Person> PeopleRepo::all() {
    std::vector<Person> out;
    sqlite3* db = Db::instance().handle();
    const char* sql = "SELECT id, full_name, rank FROM people;";

    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }

    while (sqlite3_step(st) == SQLITE_ROW) {
        Person p;
        p.id        = sqlite3_column_int64(st, 0);
        p.full_name = safe_text(st, 1);
        p.rank      = safe_text(st, 2);
        out.push_back(p);
    }
    sqlite3_finalize(st);
    return out;
}

std::optional<Person> PeopleRepo::byId(long long id) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "SELECT id, full_name, rank FROM people WHERE id = ?;";

    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }

    sqlite3_bind_int64(st, 1, id);

    if (sqlite3_step(st) == SQLITE_ROW) {
        Person p;
        p.id        = sqlite3_column_int64(st, 0);
        p.full_name = safe_text(st, 1);
        p.rank      = safe_text(st, 2);
        sqlite3_finalize(st);
        return p;
    }

    sqlite3_finalize(st);
    return std::nullopt;
}

void PeopleRepo::update(const Person& p) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "UPDATE people SET full_name=?, rank=? WHERE id=?;";

    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }

    sqlite3_bind_text(st, 1, p.full_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 2, p.rank.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(st, 3, p.id);

    if (sqlite3_step(st) != SQLITE_DONE) {
        sqlite3_finalize(st);
        throw std::runtime_error("PeopleRepo::update failed: " + std::string(sqlite3_errmsg(db)));
    }
    sqlite3_finalize(st);
}

void PeopleRepo::remove(long long id) {
    sqlite3* db = Db::instance().handle();
    
    // 1. Видаляємо залежності (екіпаж)
    const char* delCrewSql = "DELETE FROM crew_assignments WHERE person_id = ?;";
    sqlite3_stmt* stCrew = nullptr;
    if (sqlite3_prepare_v2(db, delCrewSql, -1, &stCrew, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(stCrew, 1, id);
        sqlite3_step(stCrew);
        sqlite3_finalize(stCrew);
    }

    // 2. Видаляємо людину
    const char* sql = "DELETE FROM people WHERE id=?;";
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }

    sqlite3_bind_int64(st, 1, id);

    if (sqlite3_step(st) != SQLITE_DONE) {
        sqlite3_finalize(st);
        throw std::runtime_error("PeopleRepo::remove failed: " + std::string(sqlite3_errmsg(db)));
    }
    sqlite3_finalize(st);
}