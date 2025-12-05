#pragma once
#include "domain/Person.h"
#include <utility>

class Engineer : public Person {
public:
    Engineer(std::string name, std::string specialty)
        : Person(std::move(name)), specialty_(std::move(specialty)) {}

    const std::string& specialty() const noexcept { return specialty_; }
    void setSpecialty(std::string v) { specialty_ = std::move(v); }

    std::string role() const override { return "Engineer"; }
    std::string duty() const override { return "Maintain ship systems"; }

private:
    std::string specialty_;
};
