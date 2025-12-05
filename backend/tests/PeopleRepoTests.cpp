#include <gtest/gtest.h>
#include "db/Db.h"
#include "repos/PeopleRepo.h"

// Припускаю, що в тебе є щось типу:
// struct Person {
//     int id;
//     std::string full_name;
//     std::string rank;
//     bool active; // або int, але в тестах це не критично
// };

class PeopleRepoTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Кожен тест починає з чистої БД
        Db::instance().reset();
    }
};

// 1. Базовий CRUD: create → byId → update → remove
TEST_F(PeopleRepoTest, CreateGetUpdateDeletePerson) {
    PeopleRepo repo;

    Person p;
    p.full_name = "John Doe";
    p.rank = "Captain";
    p.active = true;

    // CREATE
    auto created = repo.create(p);
    ASSERT_GT(created.id, 0) << "Після create id повинен бути > 0";

    // GET (byId)
    auto got = repo.byId(created.id);
    ASSERT_TRUE(got.has_value()) << "byId повинен повернути значення для щойно створеної людини";
    EXPECT_EQ(got->full_name, "John Doe");
    EXPECT_EQ(got->rank, "Captain");
    EXPECT_TRUE(got->active);

    // UPDATE
    created.full_name = "John A. Doe";
    created.rank = "Admiral";
    created.active = false;
    repo.update(created);

    auto updated = repo.byId(created.id);
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated->full_name, "John A. Doe");
    EXPECT_EQ(updated->rank, "Admiral");
    EXPECT_FALSE(updated->active);

    // DELETE
    repo.remove(created.id);
    auto gone = repo.byId(created.id);
    EXPECT_FALSE(gone.has_value()) << "Після remove людина не повинна знаходитися byId";
}

// 2. byId для неіснуючого id
TEST_F(PeopleRepoTest, GetNonExistingPersonReturnsEmptyOptional) {
    PeopleRepo repo;

    auto got = repo.byId(123456); // id, якого точно нема
    EXPECT_FALSE(got.has_value());
}

// 3. Delete для неіснуючого id — не падає
TEST_F(PeopleRepoTest, DeleteNonExistingPersonDoesNotThrow) {
    PeopleRepo repo;

    EXPECT_NO_THROW(repo.remove(999999));
}

// 4. all() на порожній БД → порожній список
TEST_F(PeopleRepoTest, AllOnEmptyDbReturnsEmptyVector) {
    PeopleRepo repo;

    auto all = repo.all();   // якщо метод називається інакше — підправ тут
    EXPECT_TRUE(all.empty());
}

// 5. all() повертає всіх вставлених людей
TEST_F(PeopleRepoTest, AllReturnsAllInsertedPeople) {
    PeopleRepo repo;

    Person p1;
    p1.full_name = "Alice Smith";
    p1.rank = "Officer";
    p1.active = true;

    Person p2;
    p2.full_name = "Bob Johnson";
    p2.rank = "Sailor";
    p2.active = true;

    Person p3;
    p3.full_name = "Charlie Brown";
    p3.rank = "Trainee";
    p3.active = false;

    auto c1 = repo.create(p1);
    auto c2 = repo.create(p2);
    auto c3 = repo.create(p3);

    auto all = repo.all();

    ASSERT_EQ(all.size(), 3u);

    bool hasAlice = false, hasBob = false, hasCharlie = false;
    for (const auto &person : all) {
        if (person.id == c1.id && person.full_name == "Alice Smith") hasAlice = true;
        if (person.id == c2.id && person.full_name == "Bob Johnson") hasBob = true;
        if (person.id == c3.id && person.full_name == "Charlie Brown") hasCharlie = true;
    }

    EXPECT_TRUE(hasAlice);
    EXPECT_TRUE(hasBob);
    EXPECT_TRUE(hasCharlie);
}

// 6. active-флаг коректно зберігається
TEST_F(PeopleRepoTest, ActiveFlagIsPersistedCorrectly) {
    PeopleRepo repo;

    Person activePerson;
    activePerson.full_name = "Active Guy";
    activePerson.rank = "Engineer";
    activePerson.active = true;

    Person inactivePerson;
    inactivePerson.full_name = "Inactive Guy";
    inactivePerson.rank = "Retired";
    inactivePerson.active = false;

    auto a = repo.create(activePerson);
    auto b = repo.create(inactivePerson);

    auto gotA = repo.byId(a.id);
    auto gotB = repo.byId(b.id);

    ASSERT_TRUE(gotA.has_value());
    ASSERT_TRUE(gotB.has_value());

    EXPECT_TRUE(gotA->active);
    EXPECT_FALSE(gotB->active);
}

// 7. Update для неіснуючого запису — не падає (якщо така поведінка тобі ок)
// Якщо в твоїй реалізації ти хочеш кидати exception — змінюй EXPECT_NO_THROW
TEST_F(PeopleRepoTest, UpdateNonExistingPersonDoesNotThrow) {
    PeopleRepo repo;

    Person p;
    p.id = 424242;          // id, якого нема в базі
    p.full_name = "Ghost";
    p.rank = "None";
    p.active = false;

    EXPECT_NO_THROW(repo.update(p));
}

// 8. Можна додати простий тест на кілька create підряд,
// щоб переконатися, що id зростають
TEST_F(PeopleRepoTest, CreatedPeopleHaveIncrementingIds) {
    PeopleRepo repo;

    Person p1; p1.full_name = "P1"; p1.rank = "R1"; p1.active = true;
    Person p2; p2.full_name = "P2"; p2.rank = "R2"; p2.active = true;
    Person p3; p3.full_name = "P3"; p3.rank = "R3"; p3.active = true;

    auto c1 = repo.create(p1);
    auto c2 = repo.create(p2);
    auto c3 = repo.create(p3);

    EXPECT_LT(c1.id, c2.id);
    EXPECT_LT(c2.id, c3.id);
}
