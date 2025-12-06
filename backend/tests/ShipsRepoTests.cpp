#include <gtest/gtest.h>
#include "db/Db.h"
#include "repos/ShipsRepo.h"

class ShipsRepoTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Кожен тест починається з чистої БД
        Db::instance().reset();
    }
};

// 1. Повний CRUD для одного корабля
TEST_F(ShipsRepoTest, CreateGetUpdateDeleteSingleShip) {
    ShipsRepo repo;

    Ship s;
    s.name = "Test Ship";
    s.type = "Cargo";
    s.country = "TestLand";
    s.port_id = 1;

    // CREATE
    Ship created = repo.create(s);
    ASSERT_GT(created.id, 0);
    EXPECT_EQ(created.name, "Test Ship");
    EXPECT_EQ(created.type, "Cargo");
    EXPECT_EQ(created.country, "TestLand");
    EXPECT_EQ(created.port_id, 1);

    // GET (byId)
    auto fetched = repo.byId(created.id);
    ASSERT_TRUE(fetched.has_value());
    EXPECT_EQ(fetched->id, created.id);
    EXPECT_EQ(fetched->name, "Test Ship");
    EXPECT_EQ(fetched->type, "Cargo");
    EXPECT_EQ(fetched->country, "TestLand");
    EXPECT_EQ(fetched->port_id, 1);

    // UPDATE
    created.name = "Updated Ship";
    created.type = "Tanker";
    created.country = "NewLand";
    created.port_id = 2;

    repo.update(created);

    auto updated = repo.byId(created.id);
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->name, "Updated Ship");
    EXPECT_EQ(updated->type, "Tanker");
    EXPECT_EQ(updated->country, "NewLand");
    EXPECT_EQ(updated->port_id, 2);

    // DELETE
    repo.remove(created.id);
    auto deleted = repo.byId(created.id);
    EXPECT_FALSE(deleted.has_value());
}

// 2. byId для неіснуючого id
TEST_F(ShipsRepoTest, GetNonExistingShipReturnsEmptyOptional) {
    ShipsRepo repo;

    auto fetched = repo.byId(123456); // явно неіснуючий id
    EXPECT_FALSE(fetched.has_value());
}

// 3. Delete неіснуючого корабля не падає
TEST_F(ShipsRepoTest, DeleteNonExistingShipDoesNotThrow) {
    ShipsRepo repo;
    EXPECT_NO_THROW(repo.remove(999999));
}

// 4. all() на порожній БД → порожній список
TEST_F(ShipsRepoTest, AllOnEmptyDbReturnsEmptyVector) {
    ShipsRepo repo;

    auto allShips = repo.all(); // якщо методу all() немає — видали цей тест
    EXPECT_TRUE(allShips.empty());
}

// 5. all() повертає всі додані кораблі
TEST_F(ShipsRepoTest, AllReturnsAllInsertedShips) {
    ShipsRepo repo;

    Ship s1; s1.name = "Ship A"; s1.type = "Cargo"; s1.country = "A-Land"; s1.port_id = 1;
    Ship s2; s2.name = "Ship B"; s2.type = "Tanker"; s2.country = "B-Land"; s2.port_id = 2;
    Ship s3; s3.name = "Ship C"; s3.type = "Military"; s3.country = "C-Land"; s3.port_id = 3;

    auto c1 = repo.create(s1);
    auto c2 = repo.create(s2);
    auto c3 = repo.create(s3);

    auto allShips = repo.all();
    ASSERT_EQ(allShips.size(), 3u);

    bool hasA = false, hasB = false, hasC = false;
    for (const auto &ship : allShips) {
        if (ship.id == c1.id && ship.name == "Ship A") hasA = true;
        if (ship.id == c2.id && ship.name == "Ship B") hasB = true;
        if (ship.id == c3.id && ship.name == "Ship C") hasC = true;
    }

    EXPECT_TRUE(hasA);
    EXPECT_TRUE(hasB);
    EXPECT_TRUE(hasC);
}

// 6. Перевірка port_id та інших полів збереження
TEST_F(ShipsRepoTest, PortIdAndFieldsPersistCorrectly) {
    ShipsRepo repo;

    Ship s;
    s.name = "Port Bound";
    s.type = "Ferry";
    s.country = "HarborLand";
    s.port_id = 42;

    auto created = repo.create(s);
    auto fetched = repo.byId(created.id);

    ASSERT_TRUE(fetched.has_value());
    EXPECT_EQ(fetched->port_id, 42);
    EXPECT_EQ(fetched->name, "Port Bound");
    EXPECT_EQ(fetched->type, "Ferry");
    EXPECT_EQ(fetched->country, "HarborLand");
}

// 7. Update неіснуючого корабля — не падає (якщо така поведінка задумана)
TEST_F(ShipsRepoTest, UpdateNonExistingShipDoesNotThrow) {
    ShipsRepo repo;

    Ship ghost;
    ghost.id = 777777;
    ghost.name = "Ghost Ship";
    ghost.type = "Unknown";
    ghost.country = "Nowhere";
    ghost.port_id = 0;

    EXPECT_NO_THROW(repo.update(ghost));
}

// 8. Id створених кораблів мають зростати
TEST_F(ShipsRepoTest, CreatedShipsHaveIncrementingIds) {
    ShipsRepo repo;

    Ship s1; s1.name = "Inc 1"; s1.type = "Cargo"; s1.country = "X"; s1.port_id = 1;
    Ship s2; s2.name = "Inc 2"; s2.type = "Tanker"; s2.country = "Y"; s2.port_id = 2;
    Ship s3; s3.name = "Inc 3"; s3.type = "Ferry"; s3.country = "Z"; s3.port_id = 3;

    auto c1 = repo.create(s1);
    auto c2 = repo.create(s2);
    auto c3 = repo.create(s3);

    EXPECT_LT(c1.id, c2.id);
    EXPECT_LT(c2.id, c3.id);
}
