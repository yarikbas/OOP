// PortsRepoTest.cpp
#include <gtest/gtest.h>
#include "db/Db.h"
#include "repos/PortsRepo.h"

class PortsRepoTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Повна очистка БД перед кожним тестом
        Db::instance().reset();
    }
};

// Базовий CRUD: create → byId → update → remove
TEST_F(PortsRepoTest, CreateGetUpdateDelete) {
    PortsRepo repo;

    Port p;
    p.name   = "Odesa";
    p.region = "Black Sea";
    p.lat    = 46.50;
    p.lon    = 30.73;

    // CREATE
    auto created = repo.create(p);
    ASSERT_GT(created.id, 0);
    EXPECT_EQ(created.name, "Odesa");
    EXPECT_EQ(created.region, "Black Sea");
    EXPECT_NEAR(created.lat, 46.50, 1e-9);
    EXPECT_NEAR(created.lon, 30.73, 1e-9);

    // GET byId
    auto got = repo.byId(created.id);
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(got->id, created.id);
    EXPECT_EQ(got->name, "Odesa");
    EXPECT_EQ(got->region, "Black Sea");
    EXPECT_NEAR(got->lat, 46.50, 1e-9);
    EXPECT_NEAR(got->lon, 30.73, 1e-9);

    // UPDATE
    created.name   = "Odessa";
    created.lat    = 46.51;
    created.lon    = 30.74;
    created.region = "Black Sea UA";

    repo.update(created);

    auto upd = repo.byId(created.id);
    ASSERT_TRUE(upd.has_value());
    EXPECT_EQ(upd->name, "Odessa");
    EXPECT_EQ(upd->region, "Black Sea UA");
    EXPECT_NEAR(upd->lat, 46.51, 1e-9);
    EXPECT_NEAR(upd->lon, 30.74, 1e-9);

    // DELETE
    repo.remove(created.id);
    auto gone = repo.byId(created.id);
    EXPECT_FALSE(gone.has_value());
}

// Якщо портів немає – all() повертає порожній список
TEST_F(PortsRepoTest, AllEmptyWhenNoPorts) {
    PortsRepo repo;
    auto ports = repo.all();
    EXPECT_TRUE(ports.empty());
}

// byId для неіснуючого id повертає порожній optional
TEST_F(PortsRepoTest, GetUnknownReturnsEmptyOptional) {
    PortsRepo repo;
    auto got = repo.byId(999999);  // завідомо неіснуючий id
    EXPECT_FALSE(got.has_value());
}

// all() повертає всі порти, відсортовані за region, name
TEST_F(PortsRepoTest, AllReturnsSortedByRegionAndName) {
    PortsRepo repo;

    Port p1;
    p1.name   = "Port B";
    p1.region = "Region A";
    p1.lat    = 1.0;
    p1.lon    = 1.0;

    Port p2;
    p2.name   = "Port A";
    p2.region = "Region A";
    p2.lat    = 2.0;
    p2.lon    = 2.0;

    Port p3;
    p3.name   = "Port C";
    p3.region = "Region B";
    p3.lat    = 3.0;
    p3.lon    = 3.0;

    auto c1 = repo.create(p1);
    auto c2 = repo.create(p2);
    auto c3 = repo.create(p3);

    auto ports = repo.all();
    ASSERT_EQ(ports.size(), 3u);

    // ORDER BY region, name:
    // Region A, Port A
    // Region A, Port B
    // Region B, Port C
    EXPECT_EQ(ports[0].id, c2.id);
    EXPECT_EQ(ports[0].region, "Region A");
    EXPECT_EQ(ports[0].name, "Port A");

    EXPECT_EQ(ports[1].id, c1.id);
    EXPECT_EQ(ports[1].region, "Region A");
    EXPECT_EQ(ports[1].name, "Port B");

    EXPECT_EQ(ports[2].id, c3.id);
    EXPECT_EQ(ports[2].region, "Region B");
    EXPECT_EQ(ports[2].name, "Port C");
}
