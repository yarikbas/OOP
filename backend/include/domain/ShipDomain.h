#pragma once
#include <string>
#include <utility>

class ShipBase {
public:
    ShipBase(std::string name, double tonnage)
        : name_(std::move(name)), tonnage_(tonnage) {}
    virtual ~ShipBase() = default;

    const std::string& name() const noexcept { return name_; }
    double tonnage() const noexcept { return tonnage_; }
    void setName(std::string v) { name_ = std::move(v); }
    void setTonnage(double t) { tonnage_ = t; }

    virtual std::string category() const = 0;
    virtual std::string mission()  const = 0;

private:
    std::string name_;
    double tonnage_{};
};

class CargoShip : public ShipBase {
public:
    CargoShip(std::string name, double tonnage, double capacityTons)
        : ShipBase(std::move(name), tonnage), capacityTons_(capacityTons) {}
    double capacityTons() const noexcept { return capacityTons_; }
    std::string category() const override { return "Cargo"; }
    std::string mission()  const override { return "Transport goods"; }
private:
    double capacityTons_;
};

class MilitaryShip : public ShipBase {
public:
    MilitaryShip(std::string name, double tonnage, int weapons)
        : ShipBase(std::move(name), tonnage), weapons_(weapons) {}
    int weapons() const noexcept { return weapons_; }
    std::string category() const override { return "Military"; }
    std::string mission()  const override { return "Defense and security"; }
private:
    int weapons_;
};

class ResearchShip : public ShipBase {
public:
    ResearchShip(std::string name, double tonnage, int labs)
        : ShipBase(std::move(name), tonnage), labs_(labs) {}
    int labs() const noexcept { return labs_; }
    std::string category() const override { return "Research"; }
    std::string mission()  const override { return "Scientific exploration"; }
private:
    int labs_;
};
