#pragma once
#include <string>
#include <utility>

class PortBase {
public:
    PortBase(std::string name, std::string region)
        : name_(std::move(name)), region_(std::move(region)) {}
    virtual ~PortBase() = default;

    const std::string& name() const noexcept { return name_; }
    const std::string& region() const noexcept { return region_; }
    void setName(std::string v) { name_ = std::move(v); }
    void setRegion(std::string v) { region_ = std::move(v); }

    virtual std::string kind() const { return "Seaport"; }

private:
    std::string name_;
    std::string region_;
};
