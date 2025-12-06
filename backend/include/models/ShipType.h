#pragma once

#include <cstdint>
#include <string>

struct ShipType {
    std::int64_t id{0};
    std::string  code;        // унікальний ключ, напр. "cargo"
    std::string  name;        // читабельна назва, напр. "Cargo"
    std::string  description; // опціонально
};
