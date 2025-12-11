// include/repos/VoyageExpensesRepo.h
#pragma once

#include "models/VoyageExpense.h"
#include <vector>
#include <optional>
#include <cstdint>

class VoyageExpensesRepo {
public:
    std::vector<VoyageExpense> all();
    std::vector<VoyageExpense> byVoyageId(std::int64_t voyageId);
    std::optional<VoyageExpense> byId(std::int64_t id);
    VoyageExpense create(const VoyageExpense& expense);
    void update(const VoyageExpense& expense);
    void remove(std::int64_t id);
};
