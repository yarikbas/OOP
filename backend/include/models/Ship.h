#pragma once
#include <string>
#include <cstdint>

struct Ship {
    std::int64_t id{};          // PK у таблиці ships
    std::string  name;          // Назва корабля
    std::string  type;          // Тип (cargo, tanker, passenger, military, research, tug...)
    std::string  country;       // Країна прапора

    std::int64_t port_id{};     // FK -> ports.id (0 або NULL, якщо не пришвартований)
    std::string  status{"docked"}; // Статус: docked / loading / unloading / departed

    std::int64_t company_id{};  // FK -> companies.id (0, якщо немає компанії)
};
