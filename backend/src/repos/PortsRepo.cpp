#include "repos/PortsRepo.h"
#include "db/Db.h"

std::vector<Port> PortsRepo::all() {
    std::vector<Port> out;
    sqlite3* db = Db::instance().handle();
    const char* sql = "SELECT id, name, region, lat, lon FROM ports ORDER BY region, name";
    
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) return out;

    while (sqlite3_step(st) == SQLITE_ROW) {
        Port p;
        p.id = sqlite3_column_int64(st, 0);
        p.name = (const char*)sqlite3_column_text(st, 1);
        p.region = (const char*)sqlite3_column_text(st, 2);
        p.lat = sqlite3_column_double(st, 3);
        p.lon = sqlite3_column_double(st, 4);
        out.push_back(p);
    }
    sqlite3_finalize(st);
    return out;
}

