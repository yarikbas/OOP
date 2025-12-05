#pragma once
#include "domain/Person.h"
#include <utility>

class Researcher : public Person {
public:
    Researcher(std::string name, std::string field)
        : Person(std::move(name)), field_(std::move(field)) {}

    const std::string& field() const noexcept { return field_; }
    void setField(std::string v) { field_ = std::move(v); }

    std::string role() const override { return "Researcher"; }
    std::string duty() const override { return "Conduct experiments"; }

private:
    std::string field_;
};
