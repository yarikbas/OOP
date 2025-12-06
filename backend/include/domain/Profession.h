#pragma once
#include <string>
#include <memory>
#include <algorithm>

namespace domain {

class Profession {
public:
    virtual ~Profession() = default;

    virtual std::string code() const = 0;
    virtual std::string displayName() const = 0;
    virtual std::string description() const = 0;
};

class Engineer : public Profession {
public:
    std::string code() const override {
        return "Engineer";
    }
    std::string displayName() const override {
        return "Інженер";
    }
    std::string description() const override {
        return "Відповідає за технічний стан корабля, "
               "обслуговування систем і усунення несправностей.";
    }
};

class Captain : public Profession {
public:
    std::string code() const override {
        return "Captain";
    }
    std::string displayName() const override {
        return "Капітан";
    }
    std::string description() const override {
        return "Командує судном, приймає ключові навігаційні та "
               "операційні рішення, відповідає за безпеку екіпажу.";
    }
};

class Researcher : public Profession {
public:
    std::string code() const override {
        return "Researcher";
    }
    std::string displayName() const override {
        return "Дослідник";
    }
    std::string description() const override {
        return "Планує та виконує наукові експерименти, "
               "збирає й аналізує дані під час рейсу.";
    }
};

class Soldier : public Profession {
public:
    std::string code() const override {
        return "Soldier";
    }
    std::string displayName() const override {
        return "Солдат";
    }
    std::string description() const override {
        return "Займається безпекою, обороною корабля та виконанням "
               "воєнних/охоронних завдань.";
    }
};

class UnknownProfession : public Profession {
public:
    explicit UnknownProfession(std::string rawCode)
        : rawCode_(std::move(rawCode)) {}

    std::string code() const override {
        return rawCode_;
    }
    std::string displayName() const override {
        return "Невідома професія";
    }
    std::string description() const override {
        return "Цей код професії не підтримується системою. "
               "Можливо, його потрібно додати до доменної моделі.";
    }

private:
    std::string rawCode_;
};

class ProfessionFactory {
public:
    static std::unique_ptr<Profession> fromCode(const std::string &code) {
        std::string lower = toLower(code);

        if (lower == "engineer")   return std::make_unique<Engineer>();
        if (lower == "captain")    return std::make_unique<Captain>();
        if (lower == "researcher") return std::make_unique<Researcher>();
        if (lower == "soldier")    return std::make_unique<Soldier>();

        return std::make_unique<UnknownProfession>(code);
    }

private:
    static std::string toLower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }
};

} // namespace domain
