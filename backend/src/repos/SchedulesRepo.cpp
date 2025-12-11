// src/repos/SchedulesRepo.cpp
#include "repos/SchedulesRepo.h"
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

Schedule parseSchedule(sqlite3_stmt* st) {
    Schedule s{};
    s.id = sqlite3_column_int64(st, 0);
    s.ship_id = sqlite3_column_int64(st, 1);
    s.route_name = safe_text(st, 2);
    s.from_port_id = sqlite3_column_int64(st, 3);
    s.to_port_id = sqlite3_column_int64(st, 4);
    s.departure_day_of_week = sqlite3_column_int(st, 5);
    s.departure_time = safe_text(st, 6);
    s.recurring = safe_text(st, 7);
    s.is_active = sqlite3_column_int(st, 8) != 0;
    s.notes = safe_text(st, 9);
    return s;
}

} // namespace

std::vector<Schedule> SchedulesRepo::all() {
    sqlite3* db = Db::instance().handle();
    const char* sql = "SELECT id,ship_id,route_name,from_port_id,to_port_id,"
                      "departure_day_of_week,departure_time,recurring,is_active,notes "
                      "FROM schedules ORDER BY id";
    Stmt st(db, sql);
    std::vector<Schedule> result;
    while (sqlite3_step(st.get()) == SQLITE_ROW) {
        result.push_back(parseSchedule(st.get()));
    }
    return result;
}

std::vector<Schedule> SchedulesRepo::byShipId(std::int64_t shipId) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "SELECT id,ship_id,route_name,from_port_id,to_port_id,"
                      "departure_day_of_week,departure_time,recurring,is_active,notes "
                      "FROM schedules WHERE ship_id=? ORDER BY departure_day_of_week,departure_time";
    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, shipId);
    std::vector<Schedule> result;
    while (sqlite3_step(st.get()) == SQLITE_ROW) {
        result.push_back(parseSchedule(st.get()));
    }
    return result;
}

std::vector<Schedule> SchedulesRepo::active() {
    sqlite3* db = Db::instance().handle();
    const char* sql = "SELECT id,ship_id,route_name,from_port_id,to_port_id,"
                      "departure_day_of_week,departure_time,recurring,is_active,notes "
                      "FROM schedules WHERE is_active=1 ORDER BY departure_day_of_week,departure_time";
    Stmt st(db, sql);
    std::vector<Schedule> result;
    while (sqlite3_step(st.get()) == SQLITE_ROW) {
        result.push_back(parseSchedule(st.get()));
    }
    return result;
}

std::optional<Schedule> SchedulesRepo::byId(std::int64_t id) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "SELECT id,ship_id,route_name,from_port_id,to_port_id,"
                      "departure_day_of_week,departure_time,recurring,is_active,notes "
                      "FROM schedules WHERE id=?";
    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, id);
    if (sqlite3_step(st.get()) == SQLITE_ROW) {
        return parseSchedule(st.get());
    }
    return std::nullopt;
}

Schedule SchedulesRepo::create(const Schedule& schedule) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "INSERT INTO schedules(ship_id,route_name,from_port_id,to_port_id,"
                      "departure_day_of_week,departure_time,recurring,is_active,notes) "
                      "VALUES(?,?,?,?,?,?,?,?,?)";
    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, schedule.ship_id);
    sqlite3_bind_text(st.get(), 2, schedule.route_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(st.get(), 3, schedule.from_port_id);
    sqlite3_bind_int64(st.get(), 4, schedule.to_port_id);
    sqlite3_bind_int(st.get(), 5, schedule.departure_day_of_week);
    sqlite3_bind_text(st.get(), 6, schedule.departure_time.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.get(), 7, schedule.recurring.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(st.get(), 8, schedule.is_active ? 1 : 0);
    sqlite3_bind_text(st.get(), 9, schedule.notes.c_str(), -1, SQLITE_TRANSIENT);
    
    if (sqlite3_step(st.get()) != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }
    
    Schedule result = schedule;
    result.id = sqlite3_last_insert_rowid(db);
    return result;
}

void SchedulesRepo::update(const Schedule& schedule) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "UPDATE schedules SET ship_id=?,route_name=?,from_port_id=?,to_port_id=?,"
                      "departure_day_of_week=?,departure_time=?,recurring=?,is_active=?,notes=? "
                      "WHERE id=?";
    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, schedule.ship_id);
    sqlite3_bind_text(st.get(), 2, schedule.route_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(st.get(), 3, schedule.from_port_id);
    sqlite3_bind_int64(st.get(), 4, schedule.to_port_id);
    sqlite3_bind_int(st.get(), 5, schedule.departure_day_of_week);
    sqlite3_bind_text(st.get(), 6, schedule.departure_time.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.get(), 7, schedule.recurring.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(st.get(), 8, schedule.is_active ? 1 : 0);
    sqlite3_bind_text(st.get(), 9, schedule.notes.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(st.get(), 10, schedule.id);
    
    if (sqlite3_step(st.get()) != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }
}

void SchedulesRepo::remove(std::int64_t id) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "DELETE FROM schedules WHERE id=?";
    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, id);
    if (sqlite3_step(st.get()) != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }
}
