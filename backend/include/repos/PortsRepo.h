#pragma once
#include "models/Port.h"
#include <vector>
#include <optional>

class PortsRepo {
public:
    std::vector<Port> all();
    std::optional<Port> byId(long long id);
    // Допоміжне: підібрати валідний порт або перший
    long long resolveOrFirst(long long desired);
};
