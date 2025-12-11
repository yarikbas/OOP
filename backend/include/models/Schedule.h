// include/models/Schedule.h
#pragma once

#include <string>
#include <cstdint>

struct Schedule {
    std::int64_t id{0};
    std::int64_t ship_id{0};
    std::string  route_name;
    std::int64_t from_port_id{0};
    std::int64_t to_port_id{0};
    int          departure_day_of_week{1}; // 1=Monday, 7=Sunday
    std::string  departure_time;  // HH:MM format
    bool         is_active{true};
    std::string  recurring{"weekly"}; // weekly, biweekly, monthly
    std::string  notes;
};
