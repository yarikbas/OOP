// include/models/VoyageExpense.h
#pragma once

#include <string>
#include <cstdint>

struct VoyageExpense {
    std::int64_t id{0};
    std::int64_t voyage_id{0};
    double       fuel_cost_usd{0.0};
    double       port_fees_usd{0.0};
    double       crew_wages_usd{0.0};
    double       maintenance_cost_usd{0.0};
    double       other_costs_usd{0.0};
    double       total_cost_usd{0.0};
    std::string  notes;
};
