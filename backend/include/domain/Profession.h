#pragma once

#include <cctype>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace domain {

class Profession {
public:
    virtual ~Profession() = default;

    [[nodiscard]] virtual std::string code() const = 0;
    [[nodiscard]] virtual std::string displayName() const = 0;
    [[nodiscard]] virtual std::string description() const = 0;
};

namespace detail {

inline std::string toLowerAscii(std::string_view sv) {
    std::string out;
    out.reserve(sv.size());
    for (unsigned char c : sv) {
        out.push_back(static_cast<char>(std::tolower(c)));
    }
    return out;
}

} // namespace detail

class Engineer final : public Profession {
public:
    [[nodiscard]] std::string code() const override {
        return "Engineer";
    }
    [[nodiscard]] std::string displayName() const override {
        return "Інженер";
    }
    [[nodiscard]] std::string description() const override {
        return "Відповідає за технічний стан корабля, "
               "обслуговування систем і усунення несправностей.";
    }
};

class Captain final : public Profession {
public:
    [[nodiscard]] std::string code() const override {
        return "Captain";
    }
    [[nodiscard]] std::string displayName() const override {
        return "Капітан";
    }
    [[nodiscard]] std::string description() const override {
        return "Командує судном, приймає ключові навігаційні та "
               "операційні рішення, відповідає за безпеку екіпажу.";
    }
};

class Researcher final : public Profession {
public:
    [[nodiscard]] std::string code() const override {
        return "Researcher";
    }
    [[nodiscard]] std::string displayName() const override {
        return "Дослідник";
    }
    [[nodiscard]] std::string description() const override {
        return "Планує та виконує наукові експерименти, "
               "збирає й аналізує дані під час рейсу.";
    }
};

class Soldier final : public Profession {
public:
    [[nodiscard]] std::string code() const override {
        return "Soldier";
    }
    [[nodiscard]] std::string displayName() const override {
        return "Солдат";
    }
    [[nodiscard]] std::string description() const override {
        return "Займається безпекою, обороною корабля та виконанням "
               "воєнних/охоронних завдань.";
    }
};

class UnknownProfession final : public Profession {
public:
    explicit UnknownProfession(std::string rawCode)
        : rawCode_(std::move(rawCode)) {}

    [[nodiscard]] std::string code() const override {
        return rawCode_;
    }
    [[nodiscard]] std::string displayName() const override {
        return "Невідома професія";
    }
    [[nodiscard]] std::string description() const override {
        return "Цей код професії не підтримується системою. "
               "Можливо, його потрібно додати до доменної моделі.";
    }

private:
    std::string rawCode_;
};

class ProfessionFactory {
public:
    [[nodiscard]] static std::unique_ptr<Profession> fromCode(std::string_view code) {
        std::string lower = detail::toLowerAscii(code);

        if (lower == "engineer")   return std::make_unique<Engineer>();
        if (lower == "captain")    return std::make_unique<Captain>();
        if (lower == "researcher") return std::make_unique<Researcher>();
        if (lower == "soldier")    return std::make_unique<Soldier>();

        return std::make_unique<UnknownProfession>(std::string(code));
    }

    [[nodiscard]] static std::unique_ptr<Profession> fromCode(const std::string& code) {
        return fromCode(std::string_view{code});
    }
};

} // namespace domain
