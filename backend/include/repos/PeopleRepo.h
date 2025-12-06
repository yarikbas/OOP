#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "models/Person.h"

class PeopleRepo {
public:
    using Id = std::int64_t;

    std::vector<Person> all();
    std::optional<Person> byId(Id id);
    Person create(const Person& p);
    void update(const Person& p);
    void remove(Id id);
};
