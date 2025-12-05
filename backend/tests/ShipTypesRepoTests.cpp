// ShipTypesRepoTest.cpp
#include <gtest/gtest.h>
#include "db/Db.h"
#include "repos/ShipTypesRepo.h"

class ShipTypesRepoTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Повна очистка БД перед кожним тестом
        Db::instance().reset();
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
    repo.update(created);

    auto upd = repo.byId(created.id);
    ASSERT_TRUE(upd.has_value());
    EXPECT_EQ(upd->name, "Ice Breaker");
    EXPECT_EQ(upd->description, "Arctic operations");

    // DELETE
    repo.remove(created.id);
    auto gone = repo.byId(created.id);
    EXPECT_FALSE(gone.has_value());
}

// Пошук за унікальним code (якщо є метод byCode)
TEST_F(ShipTypesRepoTest, FindByCode) {
    ShipTypesRepo repo;

    ShipType t;
    t.code = "tanker";
    t.name = "Tanker";
    t.description = "Oil carrier";

    auto created = repo.create(t);

    auto got = repo.byCode("tanker");
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(got->id, created.id);
    EXPECT_EQ(got->code, "tanker");
    EXPECT_EQ(got->name, "Tanker");
}

// Список усіх типів (якщо є метод all)
TEST_F(ShipTypesRepoTest, ListAllContainsInsertedTypes) {
    ShipTypesRepo repo;

    ShipType a;
    a.code = "cargo";
    a.name = "Cargo";
    a.description = "General cargo";

    ShipType b;
    b.code = "ferry";
    b.name = "Ferry";
    b.description = "Passengers";

    auto ca = repo.create(a);
    auto cb = repo.create(b);

    auto list = repo.all();
    // Має бути хоча б 2 записи
    ASSERT_GE(list.size(), 2u);

    bool foundCargo = false;
    bool foundFerry = false;

    for (const auto &t : list) {
        if (t.id == ca.id && t.code == "cargo") {
            foundCargo = true;
        }
        if (t.id == cb.id && t.code == "ferry") {
            foundFerry = true;
        }
    }

    EXPECT_TRUE(foundCargo);
    EXPECT_TRUE(foundFerry);
}

// Заборона дублювати code (якщо в БД стоїть UNIQUE)
// і create() при цьому кидає std::runtime_error
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

    // Якщо ти реалізуєш інший механізм помилки –
    // поміняй тип exception тут.
    EXPECT_THROW(repo.create(t2), std::runtime_error);
}
