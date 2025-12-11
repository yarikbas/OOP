// include/repos/WeatherDataRepo.h
#pragma once

#include "models/WeatherData.h"
#include <vector>
#include <optional>
#include <cstdint>

class WeatherDataRepo {
public:
    std::vector<WeatherData> all();
    std::vector<WeatherData> byPortId(std::int64_t portId);
    std::optional<WeatherData> latest(std::int64_t portId);
    std::optional<WeatherData> byId(std::int64_t id);
    WeatherData create(const WeatherData& data);
    void update(const WeatherData& data);
    void remove(std::int64_t id);
};
