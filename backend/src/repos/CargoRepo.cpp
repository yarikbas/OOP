// src/repos/CargoRepo.cpp
#include "repos/CargoRepo.h"
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

Cargo parseCargo(sqlite3_stmt* st) {
    Cargo c{};
    c.id = sqlite3_column_int64(st, 0);
    c.name = safe_text(st, 1);
    c.type = safe_text(st, 2);
    c.weight_tonnes = sqlite3_column_double(st, 3);
    c.volume_m3 = sqlite3_column_double(st, 4);
    c.value_usd = sqlite3_column_double(st, 5);
    c.origin_port_id = sqlite3_column_int64(st, 6);
    c.destination_port_id = sqlite3_column_int64(st, 7);
    c.status = safe_text(st, 8);
    c.ship_id = sqlite3_column_int64(st, 9);
    c.loaded_at = safe_text(st, 10);
    c.delivered_at = safe_text(st, 11);
    c.notes = safe_text(st, 12);
    return c;
}

} // namespace

std::vector<Cargo> CargoRepo::all() {
    sqlite3* db = Db::instance().handle();
    const char* sql = "SELECT id,name,type,weight_tonnes,volume_m3,value_usd,"
                      "origin_port_id,destination_port_id,status,IFNULL(ship_id,0),"
                      "loaded_at,delivered_at,notes FROM cargo ORDER BY id";
    Stmt st(db, sql);
    std::vector<Cargo> result;
    while (sqlite3_step(st.get()) == SQLITE_ROW) {
        result.push_back(parseCargo(st.get()));
    }
    return result;
}

std::vector<Cargo> CargoRepo::byShipId(std::int64_t shipId) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "SELECT id,name,type,weight_tonnes,volume_m3,value_usd,"
                      "origin_port_id,destination_port_id,status,IFNULL(ship_id,0),"
                      "loaded_at,delivered_at,notes FROM cargo WHERE ship_id=? ORDER BY id";
    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, shipId);
    std::vector<Cargo> result;
    while (sqlite3_step(st.get()) == SQLITE_ROW) {
        result.push_back(parseCargo(st.get()));
    }
    return result;
}

std::vector<Cargo> CargoRepo::byStatus(const std::string& status) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "SELECT id,name,type,weight_tonnes,volume_m3,value_usd,"
                      "origin_port_id,destination_port_id,status,IFNULL(ship_id,0),"
                      "loaded_at,delivered_at,notes FROM cargo WHERE status=? ORDER BY id";
    Stmt st(db, sql);
    sqlite3_bind_text(st.get(), 1, status.c_str(), -1, SQLITE_TRANSIENT);
    std::vector<Cargo> result;
    while (sqlite3_step(st.get()) == SQLITE_ROW) {
        result.push_back(parseCargo(st.get()));
    }
    return result;
}

std::optional<Cargo> CargoRepo::byId(std::int64_t id) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "SELECT id,name,type,weight_tonnes,volume_m3,value_usd,"
                      "origin_port_id,destination_port_id,status,IFNULL(ship_id,0),"
                      "loaded_at,delivered_at,notes FROM cargo WHERE id=?";
    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, id);
    if (sqlite3_step(st.get()) == SQLITE_ROW) {
        return parseCargo(st.get());
    }
    return std::nullopt;
}

Cargo CargoRepo::create(const Cargo& cargo) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "INSERT INTO cargo(name,type,weight_tonnes,volume_m3,value_usd,"
                      "origin_port_id,destination_port_id,status,ship_id,loaded_at,delivered_at,notes) "
                      "VALUES(?,?,?,?,?,?,?,?,?,?,?,?)";
    Stmt st(db, sql);
    sqlite3_bind_text(st.get(), 1, cargo.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.get(), 2, cargo.type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(st.get(), 3, cargo.weight_tonnes);
    sqlite3_bind_double(st.get(), 4, cargo.volume_m3);
    sqlite3_bind_double(st.get(), 5, cargo.value_usd);
    sqlite3_bind_int64(st.get(), 6, cargo.origin_port_id);
    sqlite3_bind_int64(st.get(), 7, cargo.destination_port_id);
    sqlite3_bind_text(st.get(), 8, cargo.status.c_str(), -1, SQLITE_TRANSIENT);
    if (cargo.ship_id > 0) {
        sqlite3_bind_int64(st.get(), 9, cargo.ship_id);
    } else {
        sqlite3_bind_null(st.get(), 9);
    }
    sqlite3_bind_text(st.get(), 10, cargo.loaded_at.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.get(), 11, cargo.delivered_at.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.get(), 12, cargo.notes.c_str(), -1, SQLITE_TRANSIENT);
    
    if (sqlite3_step(st.get()) != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }
    
    Cargo result = cargo;
    result.id = sqlite3_last_insert_rowid(db);
    return result;
}

void CargoRepo::update(const Cargo& cargo) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "UPDATE cargo SET name=?,type=?,weight_tonnes=?,volume_m3=?,value_usd=?,"
                      "origin_port_id=?,destination_port_id=?,status=?,ship_id=?,loaded_at=?,"
                      "delivered_at=?,notes=? WHERE id=?";
    Stmt st(db, sql);
    sqlite3_bind_text(st.get(), 1, cargo.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.get(), 2, cargo.type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(st.get(), 3, cargo.weight_tonnes);
    sqlite3_bind_double(st.get(), 4, cargo.volume_m3);
    sqlite3_bind_double(st.get(), 5, cargo.value_usd);
    sqlite3_bind_int64(st.get(), 6, cargo.origin_port_id);
    sqlite3_bind_int64(st.get(), 7, cargo.destination_port_id);
    sqlite3_bind_text(st.get(), 8, cargo.status.c_str(), -1, SQLITE_TRANSIENT);
    if (cargo.ship_id > 0) {
        sqlite3_bind_int64(st.get(), 9, cargo.ship_id);
    } else {
        sqlite3_bind_null(st.get(), 9);
    }
    sqlite3_bind_text(st.get(), 10, cargo.loaded_at.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.get(), 11, cargo.delivered_at.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st.get(), 12, cargo.notes.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(st.get(), 13, cargo.id);
    
    if (sqlite3_step(st.get()) != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }
}

void CargoRepo::remove(std::int64_t id) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "DELETE FROM cargo WHERE id=?";
    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, id);
    if (sqlite3_step(st.get()) != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }
}
