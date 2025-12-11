// include/models/VoyageRecord.h
#pragma once

#include <string>
#include <cstdint>

struct VoyageRecord {
    std::int64_t id{0};
    std::int64_t ship_id{0};
    std::int64_t from_port_id{0};
    std::int64_t to_port_id{0};
    std::string  departed_at;    // ISO timestamp
    std::string  arrived_at;     // ISO timestamp
    double       actual_duration_hours{0.0};
    double       planned_duration_hours{0.0};
    double       distance_km{0.0};
    double       fuel_consumed_tonnes{0.0};
    double       total_cost_usd{0.0};
    double       total_revenue_usd{0.0};
    std::string  cargo_list;     // JSON array
    std::string  crew_list;      // JSON array
    std::string  notes;
    std::string  weather_conditions;
};
