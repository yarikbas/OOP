#pragma once
#include "domain/Person.h"
#include <utility>

class Soldier : public Person {
public:
    Soldier(std::string name, std::string rank)
        : Person(std::move(name)), rank_(std::move(rank)) {}

    const std::string& rank() const noexcept { return rank_; }
    void setRank(std::string v) { rank_ = std::move(v); }

    std::string role() const override { return "Soldier"; }
    std::string duty() const override { return "Defend and protect"; }

private:
    std::string rank_;
};
