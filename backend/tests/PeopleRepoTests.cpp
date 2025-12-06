#include <gtest/gtest.h>
#include "db/Db.h"
#include "repos/PeopleRepo.h"
#include "tests/TestHelpers.h"

class PeopleRepoTest : public ::testing::Test {
protected:
    void SetUp() override { Db::instance().reset(); }
};

TEST_F(PeopleRepoTest, CreateGetUpdateDelete) {
    PeopleRepo repo;

    auto base = repo.all().size();

    Person p;
    p.full_name = "John Doe";
    p.rank = "Engineer";
    p.active = 1;

    auto created = repo.create(p);
    ASSERT_GT(created.id, 0);
    EXPECT_EQ(repo.all().size(), base + 1);

    auto got = repo.byId(created.id);
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(got->full_name, "John Doe");
    EXPECT_EQ(got->rank, "Engineer");
    EXPECT_EQ(got->active, 1);

    created.full_name = "John A. Doe";
    created.rank = "Researcher";
    created.active = 0;
    repo.update(created);

    auto upd = repo.byId(created.id);
    ASSERT_TRUE(upd.has_value());
    EXPECT_EQ(upd->full_name, "John A. Doe");
    EXPECT_EQ(upd->rank, "Researcher");
    EXPECT_EQ(upd->active, 0);

    repo.remove(created.id);
    auto gone = repo.byId(created.id);
    EXPECT_FALSE(gone.has_value());
}

TEST_F(PeopleRepoTest, GetUnknownReturnsEmptyOptional) {
    PeopleRepo repo;
    EXPECT_FALSE(repo.byId(999999).has_value());
}
