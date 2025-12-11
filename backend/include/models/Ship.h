#pragma once

#include <cstdint>
#include <string>

struct Ship {
    std::int64_t id{0};            // PK у таблиці ships
    std::string  name;            // Назва корабля
    std::string  type;            // Тип (cargo, tanker, passenger, military, research, tug...)
    std::string  country;         // Країна прапора

    std::int64_t port_id{0};      // FK -> ports.id (0 = не пришвартований)
    std::string  status{"docked"}; // docked / loading / unloading / departed

    std::int64_t company_id{0};   // FK -> companies.id (0 = немає компанії)
    
    double       speed_knots{20.0}; // Швидкість у вузлах (за замовчуванням 20)
    
    // Voyage tracking
    std::string  departed_at;     // ISO timestamp коли відплив
    std::int64_t destination_port_id{0}; // Порт призначення
    std::string  eta;             // Estimated time of arrival (ISO timestamp)
    double       voyage_distance_km{0.0}; // Відстань рейсу в км
};
