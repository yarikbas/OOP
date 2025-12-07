// include/repos/ShipsRepo.h
#pragma once

#include "models/Ship.h"

#include <optional>
#include <vector>

class ShipsRepo {
public:
    // Отримати всі кораблі
    std::vector<Ship> all();

    // Отримати кораблі за портом
    std::vector<Ship> getByPortId(long long portId);

    // Отримати корабель за id
    std::optional<Ship> byId(long long id);

    // Створити корабель
    Ship create(const Ship& sIn);

    // Оновити корабель
    void update(const Ship& s);

    // Видалити корабель
    void remove(long long id);
};
