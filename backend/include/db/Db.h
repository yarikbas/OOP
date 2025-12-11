// include/db/Db.h
#pragma once

// Forward declaration замість важкого include
struct sqlite3;
#include <string>

class Db {
public:
    static Db& instance(); // <-- без noexcept

    sqlite3* handle() const noexcept { return db_; }

    void runMigrations();  // створення/оновлення схеми
    void insertLog(const std::string &level,
                   const std::string &event_type,
                   const std::string &entity,
                   int entity_id,
                   const std::string &user,
                   const std::string &message);
    void reset();          // очистка даних для тестів

    Db(const Db&) = delete;
    Db& operator=(const Db&) = delete;
    Db(Db&&) = delete;
    Db& operator=(Db&&) = delete;

private:
    Db();
    ~Db();

    sqlite3* db_{nullptr};
};
