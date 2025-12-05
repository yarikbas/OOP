#include <gtest/gtest.h>
#include "db/Db.h"
#include "repos/ShipTypesRepo.h"

// Припускаю, що є щось типу:
// struct ShipType {
//     int id;
//     std::string code;
//     std::string name;
//     std::string description;
// };

class ShipTypesRepoTest : public ::testing::Test {
protected:
    void SetUp() override {
        Db::instance().reset();
    }
};

// 1. Базовий повний CRUD для одного типу
TEST_F(ShipTypesRepoTest, CreateGetUpdateDeleteSingleType) {
    ShipTypesRepo repo;

    ShipType t;
    t.code = "icebreaker";
    t.name = "Icebreaker";
    t.description = "Arctic ops";

    auto created = repo.create(t);
    ASSERT_GT(created.id, 0);
    EXPECT_EQ(created.code, "icebreaker");
    EXPECT_EQ(created.name, "Icebreaker");
    EXPECT_EQ(created.description, "Arctic ops");

    auto got = repo.byId(created.id);
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(got->id, created.id);
    EXPECT_EQ(got->code, "icebreaker");
    EXPECT_EQ(got->name, "Icebreaker");
    EXPECT_EQ(got->description, "Arctic ops");

    created.name = "Ice Breaker";
    created.description = "Updated description";
    repo.update(created);

    auto upd = repo.byId(created.id);
    ASSERT_TRUE(upd.has_value());
    EXPECT_EQ(upd->name, "Ice Breaker");
    EXPECT_EQ(upd->description, "Updated description");

    repo.remove(created.id);
    auto gone = repo.byId(created.id);
    EXPECT_FALSE(gone.has_value());
}

// 2. byId для неіснуючого id
TEST_F(ShipTypesRepoTest, GetNonExistingTypeReturnsEmptyOptional) {
    ShipTypesRepo repo;

    auto got = repo.byId(123456);
    EXPECT_FALSE(got.has_value());
}

// 3. Delete неіснуючого типу не кидає
TEST_F(ShipTypesRepoTest, DeleteNonExistingTypeDoesNotThrow) {
    ShipTypesRepo repo;

    EXPECT_NO_THROW(repo.remove(999999));
}

// 4. all() на порожній БД → порожній вектор
//    Якщо в тебе немає методу all(), просто видали цей тест.
TEST_F(ShipTypesRepoTest, AllOnEmptyDbReturnsEmptyVector) {
    ShipTypesRepo repo;

    auto allTypes = repo.all();
    EXPECT_TRUE(allTypes.empty());
}

// 5. all() повертає всі додані типи
TEST_F(ShipTypesRepoTest, AllReturnsAllInsertedTypes) {
    ShipTypesRepo repo;

    ShipType t1; t1.code = "cargo";     t1.name = "Cargo";     t1.description = "Cargo ship";
    ShipType t2; t2.code = "tanker";    t2.name = "Tanker";    t2.description = "Oil tanker";
    ShipType t3; t3.code = "passenger"; t3.name = "Passenger"; t3.description = "Passenger liner";

    auto c1 = repo.create(t1);
    auto c2 = repo.create(t2);
    auto c3 = repo.create(t3);

    auto allTypes = repo.all();
    ASSERT_EQ(allTypes.size(), 3u);

    bool hasCargo = false, hasTanker = false, hasPassenger = false;
    for (const auto &st : allTypes) {
        if (st.id == c1.id && st.code == "cargo")     hasCargo = true;
        if (st.id == c2.id && st.code == "tanker")    hasTanker = true;
        if (st.id == c3.id && st.code == "passenger") hasPassenger = true;
    }

    EXPECT_TRUE(hasCargo);
    EXPECT_TRUE(hasTanker);
    EXPECT_TRUE(hasPassenger);
}

// 6. Перевірка, що code / name / description зберігаються коректно
TEST_F(ShipTypesRepoTest, FieldsPersistCorrectly) {
    ShipTypesRepo repo;

    ShipType t;
    t.code = "ferry";
    t.name = "Ferry";
    t.description = "Short-distance passenger transport";

    auto created = repo.create(t);
    auto got = repo.byId(created.id);

    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(got->code, "ferry");
    EXPECT_EQ(got->name, "Ferry");
    EXPECT_EQ(got->description, "Short-distance passenger transport");
}

// 7. Update неіснуючого типу не кидає (якщо така поведінка норм для твого репо)
TEST_F(ShipTypesRepoTest, UpdateNonExistingTypeDoesNotThrow) {
    ShipTypesRepo repo;

    ShipType ghost;
    ghost.id = 777777;
    ghost.code = "ghost";
    ghost.name = "Ghost Type";
    ghost.description = "Should not exist";

    EXPECT_NO_THROW(repo.update(ghost));
}

// 8. Id створених типів зростають
TEST_F(ShipTypesRepoTest, CreatedTypesHaveIncrementingIds) {
    ShipTypesRepo repo;

    ShipType t1; t1.code = "c1"; t1.name = "Type 1"; t1.description = "First";
    ShipType t2; t2.code = "c2"; t2.name = "Type 2"; t2.description = "Second";
    ShipType t3; t3.code = "c3"; t3.name = "Type 3"; t3.description = "Third";

    auto c1 = repo.create(t1);
    auto c2 = repo.create(t2);
    auto c3 = repo.create(t3);

    EXPECT_LT(c1.id, c2.id);
    EXPECT_LT(c2.id, c3.id);
}

// 9. (Опціонально) Пошук за code, якщо є метод byCode / findByCode
//    Якщо такого методу немає — просто видали тест.
TEST_F(ShipTypesRepoTest, FindByCodeReturnsCorrectType) {
    ShipTypesRepo repo;

    ShipType t;
    t.code = "research";
    t.name = "Research Vessel";
    t.description = "Science stuff";

    auto created = repo.create(t);

    // Якщо в тебе метод називається інакше (findByCode / by_code / getByCode) — заміни тут.
    auto found = repo.byCode("research");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->id, created.id);
    EXPECT_EQ(found->name, "Research Vessel");
}
