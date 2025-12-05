#include "repos/CrewRepo.h"
#include "db/Db.h"
#include <sqlite3.h>

static inline CrewAssignment parseRow(sqlite3_stmt* st) {
    CrewAssignment a;
    a.id        = sqlite3_column_int64(st, 0);
    a.person_id = sqlite3_column_int64(st, 1);
    a.ship_id   = sqlite3_column_int64(st, 2);
    const unsigned char* s = sqlite3_column_text(st, 3);
    a.start_utc = s ? reinterpret_cast<const char*>(s) : "";
    const unsigned char* e = sqlite3_column_text(st, 4);
    if (e) a.end_utc = std::string(reinterpret_cast<const char*>(e));
    return a;
}

std::vector<CrewAssignment> CrewRepo::currentCrewByShip(long long shipId) {
    std::vector<CrewAssignment> out;
    sqlite3* db = Db::instance().handle();
    const char* sql =
        "SELECT id, person_id, ship_id, start_utc, end_utc "
        "FROM crew_assignments "
        "WHERE ship_id=? AND end_utc IS NULL "
        "ORDER BY id";
    sqlite3_stmt* st{};
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) return out;
    sqlite3_bind_int64(st, 1, shipId);
    while (sqlite3_step(st) == SQLITE_ROW) out.push_back(parseRow(st));
    sqlite3_finalize(st);
    return out;
}

std::optional<CrewAssignment> CrewRepo::assign(long long personId, long long shipId, const std::string& startUtc) {
    sqlite3* db = Db::instance().handle();

    // 1 активне призначення на людину
    sqlite3_stmt* st{};
    if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM crew_assignments WHERE person_id=? AND end_utc IS NULL", -1, &st, nullptr) != SQLITE_OK)
        return std::nullopt;
    sqlite3_bind_int64(st, 1, personId);
    int cnt = 0; if (sqlite3_step(st) == SQLITE_ROW) cnt = sqlite3_column_int(st, 0);
    sqlite3_finalize(st);
    if (cnt > 0) return std::nullopt;

    // INSERT
    if (sqlite3_prepare_v2(db, "INSERT INTO crew_assignments(person_id, ship_id, start_utc) VALUES(?,?,?)", -1, &st, nullptr) != SQLITE_OK)
        return std::nullopt;
    sqlite3_bind_int64(st, 1, personId);
    sqlite3_bind_int64(st, 2, shipId);
    sqlite3_bind_text (st, 3, startUtc.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(st) != SQLITE_DONE) { sqlite3_finalize(st); return std::nullopt; }
    sqlite3_finalize(st);

    long long id = sqlite3_last_insert_rowid(db);

    // SELECT back
    if (sqlite3_prepare_v2(db, "SELECT id, person_id, ship_id, start_utc, end_utc FROM crew_assignments WHERE id=?", -1, &st, nullptr) != SQLITE_OK)
        return std::nullopt;
    sqlite3_bind_int64(st, 1, id);
    std::optional<CrewAssignment> out;
    if (sqlite3_step(st) == SQLITE_ROW) out = parseRow(st);
    sqlite3_finalize(st);
    return out;
}

bool CrewRepo::endActiveByPerson(long long personId, const std::string& endUtc) {
    sqlite3* db = Db::instance().handle();
    sqlite3_stmt* st{};
    if (sqlite3_prepare_v2(db, "UPDATE crew_assignments SET end_utc=? WHERE person_id=? AND end_utc IS NULL", -1, &st, nullptr) != SQLITE_OK)
        return false;
    sqlite3_bind_text (st, 1, endUtc.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(st, 2, personId);
    bool ok = (sqlite3_step(st) == SQLITE_DONE);
    int changed = sqlite3_changes(db);
    sqlite3_finalize(st);
    return ok && changed > 0;
}
