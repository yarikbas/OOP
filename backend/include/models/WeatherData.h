// include/models/WeatherData.h
#pragma once

#include <string>
#include <cstdint>

struct WeatherData {
    std::int64_t id{0};
    std::int64_t port_id{0};
    std::string  timestamp;      // ISO timestamp
    double       temperature_c{0.0};
    double       wind_speed_kmh{0.0};
    double       wind_direction_deg{0.0};
    std::string  conditions;     // clear, cloudy, rainy, stormy
    double       visibility_km{10.0};
    double       wave_height_m{0.0};
    std::string  warnings;       // JSON array of warnings
};
