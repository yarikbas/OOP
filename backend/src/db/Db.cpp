#include "db/Db.h"
#include <filesystem>
#include <stdexcept>
#include <string>
#include <iostream>

static void execOrThrow(sqlite3* db, const char* sql) {
    char* err = nullptr;
    const int rc = sqlite3_exec(db, sql, nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::string msg = err ? err : "unknown";
        if (err) sqlite3_free(err);
        throw std::runtime_error("sqlite exec failed: " + msg);
    }
}

Db& Db::instance() {
    static Db inst;
    return inst;
}

Db::Db() {
    namespace fs = std::filesystem;
    fs::create_directories("data");
    const char* path = "data/app.db";
    if (sqlite3_open(path, &db_) != SQLITE_OK)
        throw std::runtime_error("sqlite open failed");
    sqlite3_exec(db_, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    runMigrations();
}

Db::~Db() {
    if (db_) sqlite3_close(db_);
}

void Db::runMigrations() {
    // --- PORTS ---
    execOrThrow(db_,
        "CREATE TABLE IF NOT EXISTS ports ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT UNIQUE NOT NULL,"
        "  region TEXT NOT NULL,"
        "  lat REAL, lon REAL"
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

    // --- ships.company_id (для старих БД додаємо стовпчик, якщо вже є — ігноруємо помилку) ---
    sqlite3_exec(db_, "ALTER TABLE ships ADD COLUMN company_id INTEGER", nullptr, nullptr, nullptr);
    sqlite3_exec(db_, "CREATE INDEX IF NOT EXISTS idx_ships_company ON ships(company_id)", nullptr, nullptr, nullptr);

    // --- COMPANY_PORTS (зв'язок компанія ↔ порт; тільки після створення companies/ports!) ---
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

    // --- seed PORTS if empty ---
    {
        sqlite3_stmt* st{};
        sqlite3_prepare_v2(db_, "SELECT count(*) FROM ports", -1, &st, nullptr);
        int cnt = 0;
        if (sqlite3_step(st) == SQLITE_ROW) cnt = sqlite3_column_int(st, 0);
        sqlite3_finalize(st);
        if (cnt == 0) {
            execOrThrow(db_,
                "INSERT INTO ports (name, region, lat, lon) VALUES "
                "('Rotterdam','Europe',51.9,4.4),"
                "('Hamburg','Europe',53.5,9.9),"
                "('Odessa','Europe',46.4,30.7),"
                "('New York','America',40.7,-74.0),"
                "('Shanghai','Asia',31.2,121.5);"
            );
            std::cout << "[Db] Ports seeded\n";
        }
    }

    // --- seed SHIPS if empty ---
    {
        sqlite3_stmt* st{};
        sqlite3_prepare_v2(db_, "SELECT count(*) FROM ships", -1, &st, nullptr);
        int sc = 0;
        if (sqlite3_step(st) == SQLITE_ROW) sc = sqlite3_column_int(st, 0);
        sqlite3_finalize(st);
        if (sc == 0) {
            execOrThrow(db_,
                "INSERT INTO ships (name, type, country, port_id) VALUES "
                "('Hetman Sahaydachny','Military','Ukraine',(SELECT id FROM ports WHERE name='Odessa')),"
                "('Mriya Sea','Cargo','Ukraine',(SELECT id FROM ports WHERE name='Odessa')),"
                "('USS Enterprise','Military','USA',(SELECT id FROM ports WHERE name='New York')),"
                "('Liberty Star','Passenger','USA',(SELECT id FROM ports WHERE name='New York')),"
                "('Cosco Hope','Cargo','China',(SELECT id FROM ports WHERE name='Shanghai')),"
                "('Red Dragon','Military','China',(SELECT id FROM ports WHERE name='Shanghai')),"
                "('Euro Queen','Passenger','Germany',(SELECT id FROM ports WHERE name='Hamburg'));"
            );
            std::cout << "[Db] Fleet seeded successfully!\n";
        }
    }
}

void Db::reset() {
    // для тестів: чистимо тільки crew & ships (довідники лишаємо)
    char* err = nullptr;
    sqlite3_exec(db_, "DELETE FROM crew_assignments;", nullptr, nullptr, &err); if (err) sqlite3_free(err);
    sqlite3_exec(db_, "DELETE FROM ships;", nullptr, nullptr, &err);           if (err) sqlite3_free(err);
    sqlite3_exec(db_, "DELETE FROM sqlite_sequence WHERE name IN ('crew_assignments','ships');", nullptr, nullptr, &err);
    if (err) sqlite3_free(err);
}
