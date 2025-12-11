// src/repos/VoyageRecordsRepo.cpp
#include "repos/VoyageRecordsRepo.h"
#include "db/Db.h"
#include <sqlite3.h>
#include <stdexcept>

namespace {

class Stmt {
public:
    Stmt(sqlite3* db, const char* sql) : db_(db) {
        if (sqlite3_prepare_v2(db_, sql, -1, &st_, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(db_));
        }
    }
    ~Stmt() { if (st_) sqlite3_finalize(st_); }
    sqlite3_stmt* get() const noexcept { return st_; }
    Stmt(const Stmt&) = delete;
    Stmt& operator=(const Stmt&) = delete;
private:
    sqlite3* db_{nullptr};
    sqlite3_stmt* st_{nullptr};
};

std::string safe_text(sqlite3_stmt* st, int col) {
    const unsigned char* t = sqlite3_column_text(st, col);
    return t ? reinterpret_cast<const char*>(t) : "";
}

VoyageRecord parseVoyageRecord(sqlite3_stmt* st) {
    VoyageRecord v{};
    v.id = sqlite3_column_int64(st, 0);
    v.ship_id = sqlite3_column_int64(st, 1);
    v.from_port_id = sqlite3_column_int64(st, 2);
    v.to_port_id = sqlite3_column_int64(st, 3);
    v.departed_at = safe_text(st, 4);
    v.arrived_at = safe_text(st, 5);
    v.actual_duration_hours = sqlite3_column_double(st, 6);
    v.planned_duration_hours = sqlite3_column_double(st, 7);
    v.distance_km = sqlite3_column_double(st, 8);
    v.fuel_consumed_tonnes = sqlite3_column_double(st, 9);
    v.total_cost_usd = sqlite3_column_double(st, 10);
    v.total_revenue_usd = sqlite3_column_double(st, 11);
    v.cargo_list = safe_text(st, 12);
    v.crew_list = safe_text(st, 13);
    v.weather_conditions = safe_text(st, 14);
    v.notes = safe_text(st, 15);
    return v;
}

} // namespace

std::vector<VoyageRecord> VoyageRecordsRepo::all() {
    sqlite3* db = Db::instance().handle();
    const char* sql = "SELECT id,ship_id,from_port_id,to_port_id,departed_at,arrived_at,"
                      "actual_duration_hours,planned_duration_hours,distance_km,fuel_consumed_tonnes,"
                      "total_cost_usd,total_revenue_usd,cargo_list,crew_list,weather_conditions,notes "
                      "FROM voyage_records ORDER BY departed_at DESC";
    Stmt st(db, sql);
    std::vector<VoyageRecord> result;
    while (sqlite3_step(st.get()) == SQLITE_ROW) {
        result.push_back(parseVoyageRecord(st.get()));
    }
    return result;
}

std::vector<VoyageRecord> VoyageRecordsRepo::byShipId(std::int64_t shipId) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "SELECT id,ship_id,from_port_id,to_port_id,departed_at,arrived_at,"
                      "actual_duration_hours,planned_duration_hours,distance_km,fuel_consumed_tonnes,"
                      "total_cost_usd,total_revenue_usd,cargo_list,crew_list,weather_conditions,notes "
                      "FROM voyage_records WHERE ship_id=? ORDER BY departed_at DESC";
    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, shipId);
    std::vector<VoyageRecord> result;
    while (sqlite3_step(st.get()) == SQLITE_ROW) {
        result.push_back(parseVoyageRecord(st.get()));
    }
    return result;
}

std::optional<VoyageRecord> VoyageRecordsRepo::byId(std::int64_t id) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "SELECT id,ship_id,from_port_id,to_port_id,departed_at,arrived_at,"
                      "actual_duration_hours,planned_duration_hours,distance_km,fuel_consumed_tonnes,"
                      "total_cost_usd,total_revenue_usd,cargo_list,crew_list,weather_conditions,notes "
                      "FROM voyage_records WHERE id=?";
    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, id);
    if (sqlite3_step(st.get()) == SQLITE_ROW) {
        return parseVoyageRecord(st.get());
    }
    return std::nullopt;
}

VoyageRecord VoyageRecordsRepo::create(const VoyageRecord& record) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "INSERT INTO voyage_records(ship_id,from_port_id,to_port_id,departed_at,arrived_at,"
                      "actual_duration_hours,planned_duration_hours,distance_km,fuel_consumed_tonnes,"
                      "total_cost_usd,total_revenue_usd,cargo_list,crew_list,weather_conditions,notes) "
                      "VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";
    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, record.ship_id);
    sqlite3_bind_int64(st.get(), 2, record.from_port_id);
    sqlite3_bind_int64(st.get(), 3, record.to_port_id);
    sqlite3_bind_text(st.get(), 4, record.departed_at.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.get(), 5, record.arrived_at.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(st.get(), 6, record.actual_duration_hours);
    sqlite3_bind_double(st.get(), 7, record.planned_duration_hours);
    sqlite3_bind_double(st.get(), 8, record.distance_km);
    sqlite3_bind_double(st.get(), 9, record.fuel_consumed_tonnes);
    sqlite3_bind_double(st.get(), 10, record.total_cost_usd);
    sqlite3_bind_double(st.get(), 11, record.total_revenue_usd);
    sqlite3_bind_text(st.get(), 12, record.cargo_list.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.get(), 13, record.crew_list.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.get(), 14, record.weather_conditions.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.get(), 15, record.notes.c_str(), -1, SQLITE_TRANSIENT);
    
    if (sqlite3_step(st.get()) != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }
    
    VoyageRecord result = record;
    result.id = sqlite3_last_insert_rowid(db);
    return result;
}

void VoyageRecordsRepo::update(const VoyageRecord& record) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "UPDATE voyage_records SET ship_id=?,from_port_id=?,to_port_id=?,departed_at=?,"
                      "arrived_at=?,actual_duration_hours=?,planned_duration_hours=?,distance_km=?,"
                      "fuel_consumed_tonnes=?,total_cost_usd=?,total_revenue_usd=?,cargo_list=?,"
                      "crew_list=?,weather_conditions=?,notes=? WHERE id=?";
    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, record.ship_id);
    sqlite3_bind_int64(st.get(), 2, record.from_port_id);
    sqlite3_bind_int64(st.get(), 3, record.to_port_id);
    sqlite3_bind_text(st.get(), 4, record.departed_at.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.get(), 5, record.arrived_at.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(st.get(), 6, record.actual_duration_hours);
    sqlite3_bind_double(st.get(), 7, record.planned_duration_hours);
    sqlite3_bind_double(st.get(), 8, record.distance_km);
    sqlite3_bind_double(st.get(), 9, record.fuel_consumed_tonnes);
    sqlite3_bind_double(st.get(), 10, record.total_cost_usd);
    sqlite3_bind_double(st.get(), 11, record.total_revenue_usd);
    sqlite3_bind_text(st.get(), 12, record.cargo_list.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.get(), 13, record.crew_list.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.get(), 14, record.weather_conditions.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.get(), 15, record.notes.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(st.get(), 16, record.id);
    
    if (sqlite3_step(st.get()) != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }
}

void VoyageRecordsRepo::remove(std::int64_t id) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "DELETE FROM voyage_records WHERE id=?";
    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, id);
    if (sqlite3_step(st.get()) != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }
}
