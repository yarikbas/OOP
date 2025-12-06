// ShipTypesRepoTest.cpp
#include <gtest/gtest.h>

#include "db/Db.h"
#include "repos/ShipTypesRepo.h"

#include <sqlite3.h>
#include <string>
#include <stdexcept>

namespace {

static void execSql(sqlite3* db, const char* sql) {
    char* err = nullptr;
    const int rc = sqlite3_exec(db, sql, nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::string msg = err ? err : "unknown sqlite error";
        if (err) sqlite3_free(err);
        throw std::runtime_error(msg);
    }
}

// Ізоляція саме для ship_types
static void resetShipTypesOnly() {
    sqlite3* db = Db::instance().handle();

    // ship_types не має FK-залежностей у твоїй схемі (ships.type — TEXT),
    // тому безпечно чистити.
    execSql(db, "DELETE FROM ship_types;");
    execSql(db, "DELETE FROM sqlite_sequence WHERE name IN ('ship_types');");
}

} // namespace

class ShipTypesRepoTest : public ::testing::Test {
protected:
    void SetUp() override {
        // чистимо ships/crew як мінімум
        Db::instance().reset();

        // і головне — чистимо ship_types, щоб сидінг не заважав тестам
        resetShipTypesOnly();
    }
};

// Базовий CRUD: create → byId → update → remove
TEST_F(ShipTypesRepoTest, CreateGetUpdateDelete) {
    ShipTypesRepo repo;

    ShipType t;
    t.code = "icebreaker";
    t.name = "Icebreaker";
    t.description = "Arctic ops";

    // CREATE
    auto created = repo.create(t);
    ASSERT_GT(created.id, 0);
    EXPECT_EQ(created.code, "icebreaker");
    EXPECT_EQ(created.name, "Icebreaker");
    EXPECT_EQ(created.description, "Arctic ops");

    // GET byId
    auto got = repo.byId(created.id);
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(got->id, created.id);
    EXPECT_EQ(got->code, "icebreaker");
    EXPECT_EQ(got->name, "Icebreaker");
    EXPECT_EQ(got->description, "Arctic ops");

    // UPDATE
    created.name = "Ice Breaker";
    created.description = "Arctic operations";
    EXPECT_NO_THROW(repo.update(created));

    auto upd = repo.byId(created.id);
    ASSERT_TRUE(upd.has_value());
    EXPECT_EQ(upd->name, "Ice Breaker");
    EXPECT_EQ(upd->description, "Arctic operations");

    // DELETE
    EXPECT_NO_THROW(repo.remove(created.id));

    auto gone = repo.byId(created.id);
    EXPECT_FALSE(gone.has_value());
}

// Пошук за унікальним code
TEST_F(ShipTypesRepoTest, FindByCode) {
    ShipTypesRepo repo;

    ShipType t;
    t.code = "tanker_test";
    t.name = "Tanker";
    t.description = "Oil carrier";

    auto created = repo.create(t);

    auto got = repo.byCode("tanker_test");
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(got->id, created.id);
    EXPECT_EQ(got->code, "tanker_test");
    EXPECT_EQ(got->name, "Tanker");
}

// Список усіх типів
TEST_F(ShipTypesRepoTest, ListAllContainsInsertedTypes) {
    ShipTypesRepo repo;

    ShipType a;
    a.code = "cargo_test";
    a.name = "Cargo";
    a.description = "General cargo";

    ShipType b;
    b.code = "ferry_test";
    b.name = "Ferry";
    b.description = "Passengers";

    auto ca = repo.create(a);
    auto cb = repo.create(b);

    auto list = repo.all();
    ASSERT_EQ(list.size(), 2u);

    bool foundCargo = false;
    bool foundFerry = false;

    for (const auto &t : list) {
        if (t.id == ca.id && t.code == "cargo_test") foundCargo = true;
        if (t.id == cb.id && t.code == "ferry_test") foundFerry = true;
    }

    EXPECT_TRUE(foundCargo);
    EXPECT_TRUE(foundFerry);
}

// Заборона дублювати code (UNIQUE)
TEST_F(ShipTypesRepoTest, DuplicateCodeShouldFail) {
    ShipTypesRepo repo;

    ShipType t1;
    t1.code = "pilot";
    t1.name = "Pilot 1";
    t1.description = "First pilot boat";

    ShipType t2;
    t2.code = "pilot"; // той самий code
    t2.name = "Pilot 2";
    t2.description = "Second pilot boat";

    repo.create(t1);
    EXPECT_THROW(repo.create(t2), std::runtime_error);
}
