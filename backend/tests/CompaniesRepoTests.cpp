#include <gtest/gtest.h>
#include "db/Db.h"
#include "repos/CompaniesRepo.h"

class CompaniesRepoTest : public ::testing::Test {
protected:
    void SetUp() override { Db::instance().reset(); }
};

TEST_F(CompaniesRepoTest, CreateGetUpdateDelete) {
    CompaniesRepo repo;

    auto base = repo.all().size();

    Company c;
    c.name = "Oceanic Shipping";

    auto created = repo.create(c);
    ASSERT_GT(created.id, 0);
    EXPECT_EQ(created.name, "Oceanic Shipping");
    EXPECT_EQ(repo.all().size(), base + 1);

    auto got = repo.byId(created.id);
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(got->name, "Oceanic Shipping");

    created.name = "Oceanic Shipping Group";
    repo.update(created);

    auto upd = repo.byId(created.id);
    ASSERT_TRUE(upd.has_value());
    EXPECT_EQ(upd->name, "Oceanic Shipping Group");

    repo.remove(created.id);
    EXPECT_FALSE(repo.byId(created.id).has_value());
}

TEST_F(CompaniesRepoTest, GetUnknownReturnsEmptyOptional) {
    CompaniesRepo repo;
    EXPECT_FALSE(repo.byId(123456).has_value());
}
