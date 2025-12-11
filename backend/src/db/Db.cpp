#include "db/Db.h"

#include <sqlite3.h>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <iostream>
#include <ctime>

namespace {

void execOrThrow(sqlite3* db, const std::string& sql) {
    char* err = nullptr;
    const int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::string msg = err ? err : sqlite3_errmsg(db);
        if (err) sqlite3_free(err);
        throw std::runtime_error("sqlite exec failed: " + msg + " | SQL: " + sql);
    }
}

int scalarInt(sqlite3* db, const std::string& sql) {
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &st, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }

    int value = 0;
    if (sqlite3_step(st) == SQLITE_ROW) {
        value = sqlite3_column_int(st, 0);
    }

    sqlite3_finalize(st);
    return value;
}

bool columnExists(sqlite3* db, const std::string& table, const std::string& column) {
    sqlite3_stmt* st = nullptr;
    const std::string sql = "PRAGMA table_info(" + table + ");";

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &st, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }

    bool found = false;
    while (sqlite3_step(st) == SQLITE_ROW) {
        const unsigned char* name = sqlite3_column_text(st, 1);
        if (name && std::string(reinterpret_cast<const char*>(name)) == column) {
            found = true;
            break;
        }
    }

    sqlite3_finalize(st);
    return found;
}

void ensureColumn(sqlite3* db,
                  const std::string& table,
                  const std::string& column,
                  const std::string& declaration) {
    if (!columnExists(db, table, column)) {
        const std::string sql =
            "ALTER TABLE " + table +
            " ADD COLUMN " + column +
            " " + declaration + ";";
        execOrThrow(db, sql);
    }
}

// СІДИ НАВМИСНО ВИМКНЕНО.
constexpr bool kEnableSeeding = false;

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

Db& Db::instance() {
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

    // --- PEOPLE ---
    execOrThrow(db_,
        "CREATE TABLE IF NOT EXISTS people ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  full_name TEXT NOT NULL,"
        "  rank TEXT,"
        "  active INTEGER DEFAULT 1"
        ");"
    );

    ensureColumn(db_, "people", "rank", "TEXT");
    ensureColumn(db_, "people", "active", "INTEGER DEFAULT 1");

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

    // апгрейди companies для старих БД
    ensureColumn(db_, "companies", "country", "TEXT");
    ensureColumn(db_, "companies", "port_id", "INTEGER");

    // --- SHIPS ---
    // ВАЖЛИВО: одразу включаємо company_id в базову схему
    execOrThrow(db_,
        "CREATE TABLE IF NOT EXISTS ships ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT NOT NULL UNIQUE,"
        "  type TEXT NOT NULL,"
        "  country TEXT NOT NULL,"
        "  port_id INTEGER,"
        "  status TEXT DEFAULT 'docked',"
        "  company_id INTEGER,"
        "  FOREIGN KEY(port_id) REFERENCES ports(id),"
        "  FOREIGN KEY(company_id) REFERENCES companies(id)"
        ");"
    );

    // --- ships schema upgrades for older app.db versions ---
    ensureColumn(db_, "ships", "type", "TEXT NOT NULL DEFAULT 'cargo'");
    ensureColumn(db_, "ships", "country", "TEXT NOT NULL DEFAULT 'Unknown'");
    ensureColumn(db_, "ships", "port_id", "INTEGER");
    ensureColumn(db_, "ships", "status", "TEXT NOT NULL DEFAULT 'docked'");
    ensureColumn(db_, "ships", "company_id", "INTEGER");
    ensureColumn(db_, "ships", "speed_knots", "REAL NOT NULL DEFAULT 20.0");
    
    // Voyage tracking columns
    ensureColumn(db_, "ships", "departed_at", "TEXT");
    ensureColumn(db_, "ships", "destination_port_id", "INTEGER");
    ensureColumn(db_, "ships", "eta", "TEXT");
    ensureColumn(db_, "ships", "voyage_distance_km", "REAL");

    execOrThrow(db_, "CREATE INDEX IF NOT EXISTS idx_ships_company ON ships(company_id);");
    execOrThrow(db_, "CREATE INDEX IF NOT EXISTS idx_ships_port ON ships(port_id);");

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

    // Якщо раніше був не-unique індекс
    execOrThrow(db_, "DROP INDEX IF EXISTS idx_crew_ship_active;");

    // 1 активне призначення на корабель
    execOrThrow(db_,
        "CREATE UNIQUE INDEX IF NOT EXISTS ux_crew_ship_active "
        "ON crew_assignments(ship_id) WHERE end_utc IS NULL;"
    );

    // 1 активне призначення на людину
    execOrThrow(db_,
        "CREATE UNIQUE INDEX IF NOT EXISTS ux_crew_person_active "
        "ON crew_assignments(person_id) WHERE end_utc IS NULL;"
    );

    execOrThrow(db_, "CREATE INDEX IF NOT EXISTS crew_ship_idx ON crew_assignments(ship_id);");
    execOrThrow(db_, "CREATE INDEX IF NOT EXISTS crew_person_idx ON crew_assignments(person_id);");

    // --- AUTO-SEEDING DISABLED ---
    if (kEnableSeeding) {
        seedPortsIfEmpty(db_);
        seedShipsIfEmpty(db_);
    }

    // --- CARGO ---
    execOrThrow(db_,
        "CREATE TABLE IF NOT EXISTS cargo ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name TEXT NOT NULL,"
        "  type TEXT NOT NULL,"
        "  weight_tonnes REAL DEFAULT 0,"
        "  volume_m3 REAL DEFAULT 0,"
        "  value_usd REAL DEFAULT 0,"
        "  origin_port_id INTEGER,"
        "  destination_port_id INTEGER,"
        "  status TEXT DEFAULT 'pending',"
        "  ship_id INTEGER,"
        "  loaded_at TEXT,"
        "  delivered_at TEXT,"
        "  notes TEXT,"
        "  FOREIGN KEY(origin_port_id) REFERENCES ports(id),"
        "  FOREIGN KEY(destination_port_id) REFERENCES ports(id),"
        "  FOREIGN KEY(ship_id) REFERENCES ships(id)"
        ");"
    );
    execOrThrow(db_, "CREATE INDEX IF NOT EXISTS idx_cargo_ship ON cargo(ship_id);");
    execOrThrow(db_, "CREATE INDEX IF NOT EXISTS idx_cargo_status ON cargo(status);");

    // --- VOYAGE RECORDS ---
    execOrThrow(db_,
        "CREATE TABLE IF NOT EXISTS voyage_records ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  ship_id INTEGER NOT NULL,"
        "  from_port_id INTEGER NOT NULL,"
        "  to_port_id INTEGER NOT NULL,"
        "  departed_at TEXT NOT NULL,"
        "  arrived_at TEXT,"
        "  actual_duration_hours REAL DEFAULT 0,"
        "  planned_duration_hours REAL DEFAULT 0,"
        "  distance_km REAL DEFAULT 0,"
        "  fuel_consumed_tonnes REAL DEFAULT 0,"
        "  total_cost_usd REAL DEFAULT 0,"
        "  total_revenue_usd REAL DEFAULT 0,"
        "  cargo_list TEXT,"
        "  crew_list TEXT,"
        "  notes TEXT,"
        "  weather_conditions TEXT,"
        "  FOREIGN KEY(ship_id) REFERENCES ships(id),"
        "  FOREIGN KEY(from_port_id) REFERENCES ports(id),"
        "  FOREIGN KEY(to_port_id) REFERENCES ports(id)"
        ");"
    );
    execOrThrow(db_, "CREATE INDEX IF NOT EXISTS idx_voyage_ship ON voyage_records(ship_id);");
    execOrThrow(db_, "CREATE INDEX IF NOT EXISTS idx_voyage_dates ON voyage_records(departed_at, arrived_at);");

    // --- VOYAGE EXPENSES ---
    execOrThrow(db_,
        "CREATE TABLE IF NOT EXISTS voyage_expenses ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  voyage_id INTEGER NOT NULL,"
        "  fuel_cost_usd REAL DEFAULT 0,"
        "  port_fees_usd REAL DEFAULT 0,"
        "  crew_wages_usd REAL DEFAULT 0,"
        "  maintenance_cost_usd REAL DEFAULT 0,"
        "  other_costs_usd REAL DEFAULT 0,"
        "  total_cost_usd REAL DEFAULT 0,"
        "  notes TEXT,"
        "  FOREIGN KEY(voyage_id) REFERENCES voyage_records(id) ON DELETE CASCADE"
        ");"
    );
    execOrThrow(db_, "CREATE INDEX IF NOT EXISTS idx_expenses_voyage ON voyage_expenses(voyage_id);");

    // --- SCHEDULES ---
    execOrThrow(db_,
        "CREATE TABLE IF NOT EXISTS schedules ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  ship_id INTEGER NOT NULL,"
        "  route_name TEXT NOT NULL,"
        "  from_port_id INTEGER NOT NULL,"
        "  to_port_id INTEGER NOT NULL,"
        "  departure_day_of_week INTEGER DEFAULT 1,"
        "  departure_time TEXT,"
        "  is_active INTEGER DEFAULT 1,"
        "  recurring TEXT DEFAULT 'weekly',"
        "  notes TEXT,"
        "  FOREIGN KEY(ship_id) REFERENCES ships(id),"
        "  FOREIGN KEY(from_port_id) REFERENCES ports(id),"
        "  FOREIGN KEY(to_port_id) REFERENCES ports(id)"
        ");"
    );
    execOrThrow(db_, "CREATE INDEX IF NOT EXISTS idx_schedules_ship ON schedules(ship_id);");
    execOrThrow(db_, "CREATE INDEX IF NOT EXISTS idx_schedules_active ON schedules(is_active);");

    // --- WEATHER DATA ---
    execOrThrow(db_,
        "CREATE TABLE IF NOT EXISTS weather_data ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  port_id INTEGER NOT NULL,"
        "  timestamp TEXT NOT NULL,"
        "  temperature_c REAL DEFAULT 0,"
        "  wind_speed_kmh REAL DEFAULT 0,"
        "  wind_direction_deg REAL DEFAULT 0,"
        "  conditions TEXT,"
        "  visibility_km REAL DEFAULT 10,"
        "  wave_height_m REAL DEFAULT 0,"
        "  warnings TEXT,"
        "  FOREIGN KEY(port_id) REFERENCES ports(id)"
        ");"
    );
    execOrThrow(db_, "CREATE INDEX IF NOT EXISTS idx_weather_port ON weather_data(port_id);");
    execOrThrow(db_, "CREATE INDEX IF NOT EXISTS idx_weather_timestamp ON weather_data(timestamp);");

    // --- LOGS ---
    execOrThrow(db_,
        "CREATE TABLE IF NOT EXISTS logs ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  ts TEXT NOT NULL,"
        "  level TEXT NOT NULL,"
        "  event_type TEXT NOT NULL,"
        "  entity TEXT,"
        "  entity_id INTEGER,"
        "  user TEXT,"
        "  message TEXT"
        ");"
    );

    execOrThrow(db_, "CREATE INDEX IF NOT EXISTS idx_logs_event_type ON logs(event_type);");
    execOrThrow(db_, "CREATE INDEX IF NOT EXISTS idx_logs_ts ON logs(ts);");
}

void Db::insertLog(const std::string& level,
                   const std::string& event_type,
                   const std::string& entity,
                   int entity_id,
                   const std::string& user,
                   const std::string& message) {
    const char* sql = "INSERT INTO logs(ts, level, event_type, entity, entity_id, user, message) VALUES (?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &st, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(db_));
    }

    // timestamp UTC ISO-8601
    std::time_t t = std::time(nullptr);
    std::tm tm = *std::gmtime(&t);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
    const std::string ts(buf);

    sqlite3_bind_text(st, 1, ts.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 2, level.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 3, event_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 4, entity.c_str(), -1, SQLITE_TRANSIENT);
    if (entity_id > 0) sqlite3_bind_int(st, 5, entity_id); else sqlite3_bind_null(st, 5);
    sqlite3_bind_text(st, 6, user.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 7, message.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(st) != SQLITE_DONE) {
        std::string err = sqlite3_errmsg(db_);
        sqlite3_finalize(st);
        throw std::runtime_error("log insert failed: " + err);
    }
    sqlite3_finalize(st);
}

void Db::reset() {
    // reset для тестів: чистимо бізнес-дані,
    // але НЕ чіпаємо ports/ship_types, щоб ShipsRepo::create не падав з нуля

    execOrThrow(db_, "BEGIN;");
    try {
        execOrThrow(db_, "DELETE FROM crew_assignments;");
        execOrThrow(db_, "DELETE FROM company_ports;");
        execOrThrow(db_, "DELETE FROM ships;");
        execOrThrow(db_, "DELETE FROM people;");
        execOrThrow(db_, "DELETE FROM companies;");

        execOrThrow(
            db_,
            "DELETE FROM sqlite_sequence WHERE name IN ("
            "'crew_assignments','company_ports','ships','people','companies'"
            ");"
        );

        execOrThrow(db_, "COMMIT;");
    } catch (...) {
        sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
        throw;
    }
}
