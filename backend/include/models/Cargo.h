// include/models/Cargo.h
#pragma once

#include <string>
#include <cstdint>

struct Cargo {
    std::int64_t id{0};
    std::string  name;
    std::string  type;            // container, bulk, liquid, passengers
    double       weight_tonnes{0.0};
    double       volume_m3{0.0};
    double       value_usd{0.0};
    std::int64_t origin_port_id{0};
    std::int64_t destination_port_id{0};
    std::string  status{"pending"}; // pending, loaded, in_transit, delivered
    std::int64_t ship_id{0};
    std::string  loaded_at;      // ISO timestamp
    std::string  delivered_at;   // ISO timestamp
    std::string  notes;
};
