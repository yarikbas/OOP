#pragma once
#include <string>
#include <utility>

class Person {
public:
    explicit Person(std::string name) : name_(std::move(name)) {}
    virtual ~Person() = default;

    const std::string& name() const noexcept { return name_; }
    void setName(std::string v) { name_ = std::move(v); }

    virtual std::string role() const = 0;
    virtual std::string duty() const = 0;

private:
    std::string name_;
};
