#pragma once
#include <sqlite3.h>

class Db {
public:
    static Db& instance();

    sqlite3* handle() const noexcept { return db_; }
    void runMigrations();  // створення/оновлення схеми
    void reset();          // очистити дані ships (для тестів)

    Db(const Db&) = delete;
    Db& operator=(const Db&) = delete;

private:
    Db();
    ~Db();
    sqlite3* db_ = nullptr;
};
