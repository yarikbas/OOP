// include/repos/PeopleRepo.h
#pragma once

#include "models/Person.h"

#include <optional>
#include <vector>

class PeopleRepo {
public:
    std::vector<Person> all();
    std::optional<Person> byId(long long id);

    Person create(const Person& p);
    void update(const Person& p);
    void remove(long long id);
};
