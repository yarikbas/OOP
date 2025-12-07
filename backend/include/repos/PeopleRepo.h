#pragma once

#include <string>
#include <vector>
#include <optional>

// Структура даних (Тільки дані, ніякої логіки)
struct Person {
    long long id = 0;
    std::string full_name;
    std::string rank;
};

// Клас для роботи з БД
class PeopleRepo {
public:
    void createTable();
    Person create(const Person& p);
    std::vector<Person> all();
    std::optional<Person> byId(long long id);
    void update(const Person& p);
    void remove(long long id);
};