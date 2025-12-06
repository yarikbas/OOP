// src/db/Db.cpp
#include "db/Db.h"

#include <sqlite3.h>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <iostream>

namespace {

void execOrThrow(sqlite3* db, const char* sql) {
    char* err = nullptr;
    const int rc = sqlite3_exec(db, sql, nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::string msg = err ? err : sqlite3_errmsg(db);
        if (err) sqlite3_free(err);
        throw std::runtime_error("sqlite exec failed: " + msg);
    }
}

int scalarInt(sqlite3* db, const char* sql) {
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }

    int value = 0;
    if (sqlite3_step(st) == SQLITE_ROW) {
        value = sqlite3_column_int(st, 0);
    }
    sqlite3_finalize(st);
    return value;
}

bool columnExists(sqlite3* db, const char* table, const char* column) {
    sqlite3_stmt* st = nullptr;
    const std::string sql = std::string("PRAGMA table_info(") + table + ");";

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &st, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }

    bool found = false;
    while (sqlite3_step(st) == SQLITE_ROW) {
        const unsigned char* name = sqlite3_column_text(st, 1); // column name
        if (name && std::string(reinterpret_cast<const char*>(name)) == column) {
            found = true;
            break;
        }
    }

    sqlite3_finalize(st);
    return found;
}


void ensureColumn(sqlite3* db, const std::string& table, const std::string& column, const std::string& declaration) {
    if (!columnExists(db, table, column)) {
        execOrThrow(db, "ALTER TABLE " + table + " ADD COLUMN " + column + " " + declaration + ";");
    }
}

void seedPortsIfEmpty(sqlite3* db) {
    const int cnt = scalarInt(db, "SELECT COUNT(*) FROM ports;");
    if (cnt != 0) return;

    execOrThrow(db, "BEGIN;");
    try {
        execOrThrow(db,
            "INSERT INTO ports (name, region, lat, lon) VALUES "
            "('Rotterdam','Europe',51.9,4.4),"
            "('Hamburg','Europe',53.5,9.9),"
            "('Odessa','Europe',46.4,30.7),"
            "('New York','America',40.7,-74.0),"
            "('Shanghai','Asia',31.2,121.5);"
        );
        execOrThrow(db, "COMMIT;");
        std::cout << "[Db] Ports seeded\n";
    } catch (...) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        throw;
    }
}

void seedShipsIfEmpty(sqlite3* db) {
    const int cnt = scalarInt(db, "SELECT COUNT(*) FROM ships;");
    if (cnt != 0) return;

    execOrThrow(db, "BEGIN;");
    try {
        execOrThrow(db,
            "INSERT INTO ships (name, type, country, port_id) VALUES "
            "('Hetman Sahaydachny','military','Ukraine',(SELECT id FROM ports WHERE name='Odessa')),"
            "('Mriya Sea','cargo','Ukraine',(SELECT id FROM ports WHERE name='Odessa')),"
            "('USS Enterprise','military','USA',(SELECT id FROM ports WHERE name='New York')),"
            "('Liberty Star','passenger','USA',(SELECT id FROM ports WHERE name='New York')),"
            "('Cosco Hope','cargo','China',(SELECT id FROM ports WHERE name='Shanghai')),"
            "('Red Dragon','military','China',(SELECT id FROM ports WHERE name='Shanghai')),"
            "('Euro Queen','passenger','Germany',(SELECT id FROM ports WHERE name='Hamburg'));"
        );
        execOrThrow(db, "COMMIT;");
        std::cout << "[Db] Fleet seeded successfully!\n";
    } catch (...) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        throw;
    }
}

} // namespace

Db& Db::instance() { // <-- без noexcept
    static Db inst;
    return inst;
}

Db::Db() {
    namespace fs = std::filesystem;
    fs::create_directories("data");

    const char* path = "data/app.db";
    if (sqlite3_open(path, &db_) != SQLITE_OK) {
        std::string msg = db_ ? sqlite3_errmsg(db_) : "sqlite open failed";
        throw std::runtime_error(msg);
    }

    execOrThrow(db_, "PRAGMA foreign_keys = ON;");

    runMigrations();
}

Db::~Db() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

void Db::runMigrations() {
    // --- PORTS ---
    execOrThrow(db_,
        "CREATE TABLE IF NOT EXISTS ports ("
        "  id     INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name   TEXT    NOT NULL UNIQUE,"
        "  region TEXT    NOT NULL,"
        "  lat    REAL    NOT NULL,"
        "  lon    REAL    NOT NULL"
        ");"
    );

    // --- SHIP TYPES ---
    execOrThrow(db_,
        "CREATE TABLE IF NOT EXISTS ship_types ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  code TEXT UNIQUE NOT NULL,"
        "  name TEXT NOT NULL,"
        "  description TEXT"
        ");"
    );

    // seed ship_types
    execOrThrow(db_,
        "INSERT OR IGNORE INTO ship_types(code,name,description) VALUES"
        "('cargo','Cargo','General cargo / container'),"
        "('military','Military','Warship / patrol'),"
        "('passenger','Passenger','Ferry / cruise'),"
        "('tanker','Tanker','Oil / LNG / chemical'),"
        "('tug','Tug','Harbor tug / service'),"
        "('research','Research','Oceanographic / survey');"
    );

    // --- PEOPLE ---
    execOrThrow(db_,
        "CREATE TABLE IF NOT EXISTS people ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  full_name TEXT NOT NULL,"
        "  rank TEXT,"
        "  active INTEGER DEFAULT 1"
        ");"
    );

    // --- SHIPS ---
    execOrThrow(db_,
        "CREATE TABLE IF NOT EXISTS ships ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT NOT NULL UNIQUE,"
        "  type TEXT NOT NULL,"
        "  country TEXT NOT NULL,"
        "  port_id INTEGER,"
        "  status TEXT DEFAULT 'docked',"
        "  FOREIGN KEY(port_id) REFERENCES ports(id)"
        ");"
    );

    // --- COMPANIES ---
    execOrThrow(db_,
        "CREATE TABLE IF NOT EXISTS companies ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name    TEXT UNIQUE NOT NULL,"
        "  country TEXT,"
        "  port_id INTEGER,"
        "  FOREIGN KEY(port_id) REFERENCES ports(id)"
        ");"
    );

    // --- ships schema upgrades for older app.db versions ---
    ensureColumn(db_, "ships", "type", "TEXT NOT NULL DEFAULT 'cargo'");
    ensureColumn(db_, "ships", "country", "TEXT NOT NULL DEFAULT 'Unknown'");
    ensureColumn(db_, "ships", "port_id", "INTEGER");
    ensureColumn(db_, "ships", "status", "TEXT NOT NULL DEFAULT 'docked'");
    ensureColumn(db_, "ships", "company_id", "INTEGER");
    execOrThrow(db_, "CREATE INDEX IF NOT EXISTS idx_ships_company ON ships(company_id);");


    // --- COMPANY_PORTS ---
    execOrThrow(db_,
        "CREATE TABLE IF NOT EXISTS company_ports ("
        "  company_id INTEGER NOT NULL,"
        "  port_id    INTEGER NOT NULL,"
        "  is_main    INTEGER NOT NULL DEFAULT 0,"
        "  PRIMARY KEY (company_id, port_id),"
        "  FOREIGN KEY(company_id) REFERENCES companies(id) ON DELETE CASCADE,"
        "  FOREIGN KEY(port_id)    REFERENCES ports(id)"
        ");"
    );

    execOrThrow(db_,
        "CREATE UNIQUE INDEX IF NOT EXISTS ux_company_main_port "
        "ON company_ports(company_id) WHERE is_main=1;"
    );

    // FIX: дужка + крапка з комою
    execOrThrow(db_,
        "CREATE INDEX IF NOT EXISTS idx_company_ports_port "
        "ON company_ports(port_id);"
    );

    execOrThrow(db_,
        "CREATE INDEX IF NOT EXISTS idx_company_ports_company "
        "ON company_ports(company_id);"
    );

    // --- CREW ASSIGNMENTS ---
    execOrThrow(db_,
        "CREATE TABLE IF NOT EXISTS crew_assignments ("
        "  id        INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  person_id INTEGER NOT NULL,"
        "  ship_id   INTEGER NOT NULL,"
        "  start_utc TEXT    NOT NULL,"
        "  end_utc   TEXT,"
        "  FOREIGN KEY(person_id) REFERENCES people(id),"
        "  FOREIGN KEY(ship_id)   REFERENCES ships(id)"
        ");"
    );

    execOrThrow(db_,
        "CREATE INDEX IF NOT EXISTS idx_crew_ship_active "
        "ON crew_assignments(ship_id) WHERE end_utc IS NULL;"
    );

    execOrThrow(db_,
        "CREATE UNIQUE INDEX IF NOT EXISTS ux_crew_person_active "
        "ON crew_assignments(person_id) WHERE end_utc IS NULL;"
    );

    execOrThrow(db_, "CREATE INDEX IF NOT EXISTS crew_ship_idx ON crew_assignments(ship_id);");
    execOrThrow(db_, "CREATE INDEX IF NOT EXISTS crew_person_idx ON crew_assignments(person_id);");

    // --- seed PORTS / SHIPS ---
    seedPortsIfEmpty(db_);
    seedShipsIfEmpty(db_);
}

void Db::reset() {
    char* err = nullptr;

    sqlite3_exec(db_, "DELETE FROM crew_assignments;", nullptr, nullptr, &err);
    if (err) { sqlite3_free(err); err = nullptr; }

    sqlite3_exec(db_, "DELETE FROM ships;", nullptr, nullptr, &err);
    if (err) { sqlite3_free(err); err = nullptr; }

    sqlite3_exec(
        db_,
        "DELETE FROM sqlite_sequence WHERE name IN ('crew_assignments','ships');",
        nullptr, nullptr, &err
    );
    if (err) { sqlite3_free(err); err = nullptr; }
}