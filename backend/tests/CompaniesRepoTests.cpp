#include <gtest/gtest.h>
#include "db/Db.h"
#include "repos/CompaniesRepo.h"

// Припускаю, що є щось типу:
// struct Company {
//     int id;
//     std::string name;
//     std::string country; // або code, або address — підлаштуй під свою модель
//     bool active;         // або int
// };

class CompaniesRepoTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Кожен тест починається з чистої БД
        Db::instance().reset();
    }
};

// 1. Базовий CRUD: create → byId → update → remove
TEST_F(CompaniesRepoTest, CreateGetUpdateDeleteCompany) {
    CompaniesRepo repo;

    Company c;
    c.name = "Oceanic Shipping";
    c.country = "Norway";  // якщо в тебе інше поле (наприклад, city), заміни
    c.active = true;

    // CREATE
    auto created = repo.create(c);
    ASSERT_GT(created.id, 0) << "Після create id компанії повинен бути > 0";

    // GET (byId)
    auto got = repo.byId(created.id);
    ASSERT_TRUE(got.has_value()) << "byId повинен повернути компанію після create";
    EXPECT_EQ(got->name, "Oceanic Shipping");
    EXPECT_EQ(got->country, "Norway");
    EXPECT_TRUE(got->active);

    // UPDATE
    created.name = "Oceanic Shipping Group";
    created.country = "Denmark";
    created.active = false;
    repo.update(created);

    auto updated = repo.byId(created.id);
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->name, "Oceanic Shipping Group");
    EXPECT_EQ(updated->country, "Denmark");
    EXPECT_FALSE(updated->active);

    // DELETE
    repo.remove(created.id);
    auto gone = repo.byId(created.id);
    EXPECT_FALSE(gone.has_value()) << "Після remove компанія не повинна знаходитися byId";
}

// 2. byId для неіснуючого id
TEST_F(CompaniesRepoTest, GetNonExistingCompanyReturnsEmptyOptional) {
    CompaniesRepo repo;

    auto got = repo.byId(123456); // очевидно неіснуючий id
    EXPECT_FALSE(got.has_value());
}

// 3. Delete для неіснуючого id — не падає
TEST_F(CompaniesRepoTest, DeleteNonExistingCompanyDoesNotThrow) {
    CompaniesRepo repo;

    EXPECT_NO_THROW(repo.remove(999999));
}

// 4. all() на порожній БД → порожній список
TEST_F(CompaniesRepoTest, AllOnEmptyDbReturnsEmptyVector) {
    CompaniesRepo repo;

    auto all = repo.all();   // якщо метод називається інакше — підправ
    EXPECT_TRUE(all.empty());
}

// 5. all() повертає всі вставлені компанії
TEST_F(CompaniesRepoTest, AllReturnsAllInsertedCompanies) {
    CompaniesRepo repo;

    Company c1;
    c1.name = "Blue Ocean";
    c1.country = "Greece";
    c1.active = true;

    Company c2;
    c2.name = "Black Sea Lines";
    c2.country = "Ukraine";
    c2.active = true;

    Company c3;
    c3.name = "Retired Shipping";
    c3.country = "UK";
    c3.active = false;

    auto created1 = repo.create(c1);
    auto created2 = repo.create(c2);
    auto created3 = repo.create(c3);

    auto all = repo.all();

    ASSERT_EQ(all.size(), 3u);

    bool hasBlueOcean = false, hasBlackSea = false, hasRetired = false;
    for (const auto &company : all) {
        if (company.id == created1.id && company.name == "Blue Ocean") hasBlueOcean = true;
        if (company.id == created2.id && company.name == "Black Sea Lines") hasBlackSea = true;
        if (company.id == created3.id && company.name == "Retired Shipping") hasRetired = true;
    }

    EXPECT_TRUE(hasBlueOcean);
    EXPECT_TRUE(hasBlackSea);
    EXPECT_TRUE(hasRetired);
}

// 6. active-флаг коректно зберігається
TEST_F(CompaniesRepoTest, ActiveFlagIsPersistedCorrectly) {
    CompaniesRepo repo;

    Company activeCompany;
    activeCompany.name = "Active Co";
    activeCompany.country = "Netherlands";
    activeCompany.active = true;

    Company inactiveCompany;
    inactiveCompany.name = "Inactive Co";
    inactiveCompany.country = "Germany";
    inactiveCompany.active = false;

    auto a = repo.create(activeCompany);
    auto b = repo.create(inactiveCompany);

    auto gotA = repo.byId(a.id);
    auto gotB = repo.byId(b.id);

    ASSERT_TRUE(gotA.has_value());
    ASSERT_TRUE(gotB.has_value());

    EXPECT_TRUE(gotA->active);
    EXPECT_FALSE(gotB->active);
}

// 7. Update для неіснуючого запису — не падає (якщо така поведінка ок)
// Якщо ти хочеш, щоб при цьому кидався exception — тут міняємо EXPECT_NO_THROW
TEST_F(CompaniesRepoTest, UpdateNonExistingCompanyDoesNotThrow) {
    CompaniesRepo repo;

    Company ghost;
    ghost.id = 424242;          // id, якого нема в базі
    ghost.name = "Ghost Co";
    ghost.country = "Nowhere";
    ghost.active = false;

    EXPECT_NO_THROW(repo.update(ghost));
}

// 8. Кілька create → id мають зростати
TEST_F(CompaniesRepoTest, CreatedCompaniesHaveIncrementingIds) {
    CompaniesRepo repo;

    Company c1; c1.name = "C1"; c1.country = "X"; c1.active = true;
    Company c2; c2.name = "C2"; c2.country = "Y"; c2.active = true;
    Company c3; c3.name = "C3"; c3.country = "Z"; c3.active = true;

    auto created1 = repo.create(c1);
    auto created2 = repo.create(c2);
    auto created3 = repo.create(c3);

    EXPECT_LT(created1.id, created2.id);
    EXPECT_LT(created2.id, created3.id);
}
