#include <gtest/gtest.h>
#include "db/Db.h"
#include "repos/PortsRepo.h"
#include "repos/ShipsRepo.h"
#include "tests/TestHelpers.h"

class PortsRepoTest : public ::testing::Test {
protected:
    void SetUp() override { Db::instance().reset(); }
};

TEST_F(PortsRepoTest, CreateGetUpdateDelete) {
    PortsRepo repo;

    auto base = repo.all().size();

    Port p;
    p.name   = "Odesa_Test";
    p.region = "Black Sea";
    p.lat    = 46.50;
    p.lon    = 30.73;

    auto created = repo.create(p);
    ASSERT_GT(created.id, 0);

    auto afterCreate = repo.all();
    EXPECT_EQ(afterCreate.size(), base + 1);

    auto got = repo.getById(created.id);
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(got->name, "Odesa_Test");
    EXPECT_EQ(got->region, "Black Sea");

    created.name   = "Odesa_Test_Upd";
    created.region = "Black Sea UA";
    created.lat    = 46.51;
    created.lon    = 30.74;

    EXPECT_TRUE(repo.update(created));

    auto upd = repo.getById(created.id);
    ASSERT_TRUE(upd.has_value());
    EXPECT_EQ(upd->name, "Odesa_Test_Upd");
    EXPECT_EQ(upd->region, "Black Sea UA");

    EXPECT_TRUE(repo.remove(created.id));
    auto gone = repo.getById(created.id);
    EXPECT_FALSE(gone.has_value());
}

TEST_F(PortsRepoTest, UpdateUnknownReturnsFalse) {
    PortsRepo repo;
    Port p;
    p.id = 999999;
    p.name = "X";
    p.region = "Y";
    p.lat = 1;
    p.lon = 1;

    EXPECT_FALSE(repo.update(p));
}

TEST_F(PortsRepoTest, RemoveUnknownReturnsFalse) {
    PortsRepo repo;
    EXPECT_FALSE(repo.remove(999999));
}

TEST_F(PortsRepoTest, CannotRemovePortReferencedByShip) {
    // Створюємо порт і корабель, що на нього посилається
    auto port = ensurePort("Port_FK_Test", "Region_FK", 10.0, 20.0);

    ShipsRepo ships;
    Ship s;
    s.name = "Ship_FK_Test";
    s.type = "cargo";
    s.country = "UA";
    s.port_id = port.id;
    s.status = "docked";
    s.company_id = 0;
    auto createdShip = ships.create(s);
    ASSERT_GT(createdShip.id, 0);

    PortsRepo ports;
    // при FK constraint step поверне не SQLITE_DONE -> remove має повернути false
    EXPECT_FALSE(ports.remove(port.id));
}
