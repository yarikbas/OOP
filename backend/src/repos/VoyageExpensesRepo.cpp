// src/repos/VoyageExpensesRepo.cpp
#include "repos/VoyageExpensesRepo.h"
#include "db/Db.h"
#include <sqlite3.h>
#include <stdexcept>

namespace {

class Stmt {
public:
    Stmt(sqlite3* db, const char* sql) : db_(db) {
        if (sqlite3_prepare_v2(db_, sql, -1, &st_, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(db_));
        }
    }
    ~Stmt() { if (st_) sqlite3_finalize(st_); }
    sqlite3_stmt* get() const noexcept { return st_; }
    Stmt(const Stmt&) = delete;
    Stmt& operator=(const Stmt&) = delete;
private:
    sqlite3* db_{nullptr};
    sqlite3_stmt* st_{nullptr};
};

std::string safe_text(sqlite3_stmt* st, int col) {
    const unsigned char* t = sqlite3_column_text(st, col);
    return t ? reinterpret_cast<const char*>(t) : "";
}

VoyageExpense parseVoyageExpense(sqlite3_stmt* st) {
    VoyageExpense e{};
    e.id = sqlite3_column_int64(st, 0);
    e.voyage_id = sqlite3_column_int64(st, 1);
    e.fuel_cost_usd = sqlite3_column_double(st, 2);
    e.port_fees_usd = sqlite3_column_double(st, 3);
    e.crew_wages_usd = sqlite3_column_double(st, 4);
    e.maintenance_cost_usd = sqlite3_column_double(st, 5);
    e.other_costs_usd = sqlite3_column_double(st, 6);
    e.total_cost_usd = sqlite3_column_double(st, 7);
    e.notes = safe_text(st, 8);
    return e;
}

} // namespace

std::vector<VoyageExpense> VoyageExpensesRepo::all() {
    sqlite3* db = Db::instance().handle();
    const char* sql = "SELECT id,voyage_id,fuel_cost_usd,port_fees_usd,crew_wages_usd,"
                      "maintenance_cost_usd,other_costs_usd,total_cost_usd,notes "
                      "FROM voyage_expenses ORDER BY id";
    Stmt st(db, sql);
    std::vector<VoyageExpense> result;
    while (sqlite3_step(st.get()) == SQLITE_ROW) {
        result.push_back(parseVoyageExpense(st.get()));
    }
    return result;
}

std::vector<VoyageExpense> VoyageExpensesRepo::byVoyageId(std::int64_t voyageId) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "SELECT id,voyage_id,fuel_cost_usd,port_fees_usd,crew_wages_usd,"
                      "maintenance_cost_usd,other_costs_usd,total_cost_usd,notes "
                      "FROM voyage_expenses WHERE voyage_id=? ORDER BY id";
    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, voyageId);
    std::vector<VoyageExpense> result;
    while (sqlite3_step(st.get()) == SQLITE_ROW) {
        result.push_back(parseVoyageExpense(st.get()));
    }
    return result;
}

std::optional<VoyageExpense> VoyageExpensesRepo::byId(std::int64_t id) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "SELECT id,voyage_id,fuel_cost_usd,port_fees_usd,crew_wages_usd,"
                      "maintenance_cost_usd,other_costs_usd,total_cost_usd,notes "
                      "FROM voyage_expenses WHERE id=?";
    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, id);
    if (sqlite3_step(st.get()) == SQLITE_ROW) {
        return parseVoyageExpense(st.get());
    }
    return std::nullopt;
}

VoyageExpense VoyageExpensesRepo::create(const VoyageExpense& expense) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "INSERT INTO voyage_expenses(voyage_id,fuel_cost_usd,port_fees_usd,crew_wages_usd,"
                      "maintenance_cost_usd,other_costs_usd,total_cost_usd,notes) VALUES(?,?,?,?,?,?,?,?)";
    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, expense.voyage_id);
    sqlite3_bind_double(st.get(), 2, expense.fuel_cost_usd);
    sqlite3_bind_double(st.get(), 3, expense.port_fees_usd);
    sqlite3_bind_double(st.get(), 4, expense.crew_wages_usd);
    sqlite3_bind_double(st.get(), 5, expense.maintenance_cost_usd);
    sqlite3_bind_double(st.get(), 6, expense.other_costs_usd);
    sqlite3_bind_double(st.get(), 7, expense.total_cost_usd);
    sqlite3_bind_text(st.get(), 8, expense.notes.c_str(), -1, SQLITE_TRANSIENT);
    
    if (sqlite3_step(st.get()) != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }
    
    VoyageExpense result = expense;
    result.id = sqlite3_last_insert_rowid(db);
    return result;
}

void VoyageExpensesRepo::update(const VoyageExpense& expense) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "UPDATE voyage_expenses SET voyage_id=?,fuel_cost_usd=?,port_fees_usd=?,"
                      "crew_wages_usd=?,maintenance_cost_usd=?,other_costs_usd=?,total_cost_usd=?,notes=? "
                      "WHERE id=?";
    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, expense.voyage_id);
    sqlite3_bind_double(st.get(), 2, expense.fuel_cost_usd);
    sqlite3_bind_double(st.get(), 3, expense.port_fees_usd);
    sqlite3_bind_double(st.get(), 4, expense.crew_wages_usd);
    sqlite3_bind_double(st.get(), 5, expense.maintenance_cost_usd);
    sqlite3_bind_double(st.get(), 6, expense.other_costs_usd);
    sqlite3_bind_double(st.get(), 7, expense.total_cost_usd);
    sqlite3_bind_text(st.get(), 8, expense.notes.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(st.get(), 9, expense.id);
    
    if (sqlite3_step(st.get()) != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }
}

void VoyageExpensesRepo::remove(std::int64_t id) {
    sqlite3* db = Db::instance().handle();
    const char* sql = "DELETE FROM voyage_expenses WHERE id=?";
    Stmt st(db, sql);
    sqlite3_bind_int64(st.get(), 1, id);
    if (sqlite3_step(st.get()) != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }
}
