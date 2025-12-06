#include <gtest/gtest.h>
#include "db/Db.h"
#include "repos/ShipTypesRepo.h"

class ShipTypesRepoTest : public ::testing::Test {
protected:
    void SetUp() override { Db::instance().reset(); }
};

TEST_F(ShipTypesRepoTest, CreateGetUpdateDelete) {
    ShipTypesRepo repo;

    ShipType t;
    t.code = "icebreaker";
    t.name = "Icebreaker";
    t.description = "Arctic ops";

    auto created = repo.create(t);
    ASSERT_GT(created.id, 0);

    auto got = repo.byId(created.id);
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(got->code, "icebreaker");

    auto byCode = repo.byCode("icebreaker");
    ASSERT_TRUE(byCode.has_value());
    EXPECT_EQ(byCode->id, created.id);

    created.name = "Ice Breaker";
    created.description = "Arctic operations";
    repo.update(created);

    auto upd = repo.byId(created.id);
    ASSERT_TRUE(upd.has_value());
    EXPECT_EQ(upd->name, "Ice Breaker");

    repo.remove(created.id);
    EXPECT_FALSE(repo.byId(created.id).has_value());
}

TEST_F(ShipTypesRepoTest, DuplicateCodeThrows) {
    ShipTypesRepo repo;

    ShipType t1;
    t1.code = "pilot";
    t1.name = "Pilot 1";
    t1.description = "First";

    ShipType t2;
    t2.code = "pilot";
    t2.name = "Pilot 2";
    t2.description = "Second";

    repo.create(t1);
    EXPECT_THROW(repo.create(t2), std::runtime_error);
}
