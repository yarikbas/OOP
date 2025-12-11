// src/repos/WeatherDataRepo.cpp
#include "repos/WeatherDataRepo.h"
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

WeatherData parseWeatherData(sqlite3_stmt* st) {
    WeatherData w{};
    w.id = sqlite3_column_int64(st, 0);
    w.port_id = sqlite3_column_int64(st, 1);
    w.timestamp = safe_text(st, 2);
    w.temperature_c = sqlite3_column_double(st, 3);
    w.wind_speed_kmh = sqlite3_column_double(st, 4);
    w.wind_direction_deg = sqlite3_column_int(st, 5);
    w.conditions = safe_text(st, 6);
    w.visibility_km = sqlite3_column_double(st, 7);
    w.wave_height_m = sqlite3_column_double(st, 8);
    w.warnings = safe_text(st, 9);
    return w;
}

} // namespace

std::vector<WeatherData> WeatherDataRepo::all() {
    sqlite3* db = Db::instance().handle();
    const char* sql = "SELECT id,port_id,timestamp,temperature_c,wind_speed_kmh,wind_direction_deg,"
                      "conditions,visibility_km,wave_height_m,warnings "
                      "FROM weather_data ORDER BY timestamp DESC";
    Stmt st(db, sql);
    std::vector<WeatherData> result;
    while (sqlite3_step(st.get()) == SQLITE_ROW) {
        result.push_back(parseWeatherData(st.get()));
    }
    return result;
}

std::vector<WeatherData> WeatherDataRepo::byPortId(std::int64_t portId) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "SELECT id,port_id,timestamp,temperature_c,wind_speed_kmh,wind_direction_deg,"
                      "conditions,visibility_km,wave_height_m,warnings "
                      "FROM weather_data WHERE port_id=? ORDER BY timestamp DESC";
    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, portId);
    std::vector<WeatherData> result;
    while (sqlite3_step(st.get()) == SQLITE_ROW) {
        result.push_back(parseWeatherData(st.get()));
    }
    return result;
}

std::optional<WeatherData> WeatherDataRepo::latest(std::int64_t portId) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "SELECT id,port_id,timestamp,temperature_c,wind_speed_kmh,wind_direction_deg,"
                      "conditions,visibility_km,wave_height_m,warnings "
                      "FROM weather_data WHERE port_id=? ORDER BY timestamp DESC LIMIT 1";
    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, portId);
    if (sqlite3_step(st.get()) == SQLITE_ROW) {
        return parseWeatherData(st.get());
    }
    return std::nullopt;
}

std::optional<WeatherData> WeatherDataRepo::byId(std::int64_t id) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "SELECT id,port_id,timestamp,temperature_c,wind_speed_kmh,wind_direction_deg,"
                      "conditions,visibility_km,wave_height_m,warnings "
                      "FROM weather_data WHERE id=?";
    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, id);
    if (sqlite3_step(st.get()) == SQLITE_ROW) {
        return parseWeatherData(st.get());
    }
    return std::nullopt;
}

WeatherData WeatherDataRepo::create(const WeatherData& data) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "INSERT INTO weather_data(port_id,timestamp,temperature_c,wind_speed_kmh,"
                      "wind_direction_deg,conditions,visibility_km,wave_height_m,warnings) "
                      "VALUES(?,?,?,?,?,?,?,?,?)";
    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, data.port_id);
    sqlite3_bind_text(st.get(), 2, data.timestamp.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(st.get(), 3, data.temperature_c);
    sqlite3_bind_double(st.get(), 4, data.wind_speed_kmh);
    sqlite3_bind_int(st.get(), 5, data.wind_direction_deg);
    sqlite3_bind_text(st.get(), 6, data.conditions.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(st.get(), 7, data.visibility_km);
    sqlite3_bind_double(st.get(), 8, data.wave_height_m);
    sqlite3_bind_text(st.get(), 9, data.warnings.c_str(), -1, SQLITE_TRANSIENT);
    
    if (sqlite3_step(st.get()) != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }
    
    WeatherData result = data;
    result.id = sqlite3_last_insert_rowid(db);
    return result;
}

void WeatherDataRepo::update(const WeatherData& data) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "UPDATE weather_data SET port_id=?,timestamp=?,temperature_c=?,wind_speed_kmh=?,"
                      "wind_direction_deg=?,conditions=?,visibility_km=?,wave_height_m=?,warnings=? "
                      "WHERE id=?";
    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, data.port_id);
    sqlite3_bind_text(st.get(), 2, data.timestamp.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(st.get(), 3, data.temperature_c);
    sqlite3_bind_double(st.get(), 4, data.wind_speed_kmh);
    sqlite3_bind_int(st.get(), 5, data.wind_direction_deg);
    sqlite3_bind_text(st.get(), 6, data.conditions.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(st.get(), 7, data.visibility_km);
    sqlite3_bind_double(st.get(), 8, data.wave_height_m);
    sqlite3_bind_text(st.get(), 9, data.warnings.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(st.get(), 10, data.id);
    
    if (sqlite3_step(st.get()) != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }
}

void WeatherDataRepo::remove(std::int64_t id) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "DELETE FROM weather_data WHERE id=?";
    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, id);
    if (sqlite3_step(st.get()) != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }
}
