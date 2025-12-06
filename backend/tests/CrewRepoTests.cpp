#include <gtest/gtest.h>
#include "db/Db.h"
#include "repos/CrewRepo.h"
#include "repos/PeopleRepo.h"
#include "repos/ShipsRepo.h"
#include "repos/PortsRepo.h"
#include "tests/TestHelpers.h"

class CrewRepoTest : public ::testing::Test {
protected:
    void SetUp() override { Db::instance().reset(); }
};

TEST_F(CrewRepoTest, AssignAndListCurrentCrew) {
    auto port = ensurePort("CrewPort", "R", 1, 1);

    PeopleRepo people;
    ShipsRepo ships;
    CrewRepo crew;

    Person p; p.full_name = "Alice"; p.rank = "Engineer"; p.active = 1;
    auto createdP = people.create(p);

    Ship s; 
    s.name="TestShip"; s.type="research"; s.country="UA";
    s.port_id=port.id; s.status="docked"; s.company_id=0;
    auto createdS = ships.create(s);

    auto a = crew.assign(createdP.id, createdS.id, "2025-01-01T00:00:00Z");
    ASSERT_TRUE(a.has_value());

    auto list = crew.currentCrewByShip(createdS.id);
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0].person_id, createdP.id);
}

TEST_F(CrewRepoTest, PreventSecondActiveAssignmentForPerson) {
    auto port = ensurePort("CrewPort2", "R", 1, 1);

    PeopleRepo people;
    ShipsRepo ships;
    CrewRepo crew;

    Person p; p.full_name = "Bob"; p.rank = "Engineer"; p.active = 1;
    auto createdP = people.create(p);

    Ship s1; s1.name="S1"; s1.type="cargo"; s1.country="UA";
    s1.port_id=port.id; s1.status="docked"; s1.company_id=0;

    Ship s2; s2.name="S2"; s2.type="cargo"; s2.country="UA";
    s2.port_id=port.id; s2.status="docked"; s2.company_id=0;

    auto c1 = ships.create(s1);
    auto c2 = ships.create(s2);

    ASSERT_TRUE(crew.assign(createdP.id, c1.id, "2025-01-01T00:00:00Z").has_value());
    ASSERT_FALSE(crew.assign(createdP.id, c2.id, "2025-01-02T00:00:00Z").has_value());
}

TEST_F(CrewRepoTest, PreventSecondActiveAssignmentForShip) {
    auto port = ensurePort("CrewPort3", "R", 1, 1);

    PeopleRepo people;
    ShipsRepo ships;
    CrewRepo crew;

    auto p1 = ensurePerson("P1", "Engineer", 1);
    auto p2 = ensurePerson("P2", "Engineer", 1);

    Ship s; 
    s.name="OneShip"; s.type="cargo"; s.country="UA";
    s.port_id=port.id; s.status="docked"; s.company_id=0;

    auto ship = ships.create(s);

    ASSERT_TRUE(crew.assign(p1.id, ship.id, "2025-01-01T00:00:00Z").has_value());
    // друге активне призначення на той самий ship має бути заборонене
    ASSERT_FALSE(crew.assign(p2.id, ship.id, "2025-01-02T00:00:00Z").has_value());
}

TEST_F(CrewRepoTest, EndAssignmentAllowsReassign) {
    auto port = ensurePort("CrewPort4", "R", 1, 1);

    PeopleRepo people;
    ShipsRepo ships;
    CrewRepo crew;

    auto p = ensurePerson("Reassign Person", "Engineer", 1);

    Ship s1; s1.name="S1"; s1.type="cargo"; s1.country="UA";
    s1.port_id=port.id; s1.status="docked"; s1.company_id=0;

    Ship s2; s2.name="S2"; s2.type="cargo"; s2.country="UA";
    s2.port_id=port.id; s2.status="docked"; s2.company_id=0;

    auto ship1 = ships.create(s1);
    auto ship2 = ships.create(s2);

    auto a1 = crew.assign(p.id, ship1.id, "2025-01-01T00:00:00Z");
    ASSERT_TRUE(a1.has_value());

    // завершуємо
    auto ended = crew.end(p.id, "2025-01-10T00:00:00Z");
    ASSERT_TRUE(ended.has_value());

    // тепер можна призначити на інший корабель
    auto a2 = crew.assign(p.id, ship2.id, "2025-01-11T00:00:00Z");
    ASSERT_TRUE(a2.has_value());
}
