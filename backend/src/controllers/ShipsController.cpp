#include "controllers/ShipsController.h"
#include "repos/ShipsRepo.h"
#include "db/Db.h"

#include <drogon/drogon.h>
#include <json/json.h>
#include <sqlite3.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace {

using drogon::HttpRequestPtr;
using drogon::HttpResponse;
using drogon::HttpResponsePtr;
using drogon::HttpStatusCode;

HttpResponsePtr jsonError(const std::string& msg,
                          HttpStatusCode code,
                          const std::string& details = {}) {
    Json::Value e;
    e["error"] = msg;
    if (!details.empty()) {
        e["details"] = details; // корисно під час налагодження
    }
    auto r = HttpResponse::newHttpJsonResponse(e);
    r->setStatusCode(code);
    return r;
}

Json::Value shipToJson(const Ship& s) {
    Json::Value j;
    j["id"]         = Json::Int64(s.id);
    j["name"]       = s.name;
    j["type"]       = s.type;
    j["country"]    = s.country;
    j["port_id"]    = Json::Int64(s.port_id);
    j["status"]     = s.status;
    j["company_id"] = Json::Int64(s.company_id);
    return j;
}

// ------------ СТАТУСИ ------------

constexpr std::array<std::string_view, 4> kShipStatuses = {
    "docked",
    "loading",
    "unloading",
    "departed"
};

bool isValidStatus(std::string_view s) {
    return std::any_of(kShipStatuses.begin(), kShipStatuses.end(),
                       [s](std::string_view v) { return v == s; });
}

HttpResponsePtr invalidStatusResponse(std::string_view status) {
    Json::Value err;
    err["error"]  = "invalid status";
    err["status"] = std::string(status);

    Json::Value allowed(Json::arrayValue);
    for (auto st : kShipStatuses) {
        allowed.append(std::string(st));
    }
    err["allowed"] = allowed;

    auto r = HttpResponse::newHttpJsonResponse(err);
    r->setStatusCode(drogon::k400BadRequest);
    return r;
}

// ------------ РАНГИ (UA + EN) ------------

constexpr std::string_view kCaptainUa    = "Капітан";
constexpr std::string_view kCaptainEn    = "Captain";
constexpr std::string_view kEngineerUa   = "Інженер";
constexpr std::string_view kEngineerEn   = "Engineer";
constexpr std::string_view kResearcherUa = "Дослідник";
constexpr std::string_view kResearcherEn = "Researcher";
constexpr std::string_view kSoldierUa    = "Солдат";
constexpr std::string_view kSoldierEn    = "Soldier";

// ------------ SQL ХЕЛПЕРИ ДЛЯ ЕКІПАЖУ ------------

int countActiveCrew(sqlite3* db, std::int64_t shipId) {
    sqlite3_stmt* st = nullptr;
    const char* sql =
        "SELECT COUNT(*) FROM crew_assignments "
        "WHERE ship_id = ? AND end_utc IS NULL;";

    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }

    sqlite3_bind_int64(st, 1, shipId);

    int cnt = 0;
    if (sqlite3_step(st) == SQLITE_ROW) {
        cnt = sqlite3_column_int(st, 0);
    }

    sqlite3_finalize(st);
    return cnt;
}

int countActiveCrewWithRank2(sqlite3* db,
                             std::int64_t shipId,
                             std::string_view rank1,
                             std::string_view rank2) {
    sqlite3_stmt* st = nullptr;
    const char* sql =
        "SELECT COUNT(*) "
        "FROM crew_assignments c "
        "JOIN people p ON p.id = c.person_id "
        "WHERE c.ship_id = ? "
        "  AND c.end_utc IS NULL "
        "  AND (p.rank = ? OR p.rank = ?);";

    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }

    sqlite3_bind_int64(st, 1, shipId);
    sqlite3_bind_text(st, 2, rank1.data(), static_cast<int>(rank1.size()), SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 3, rank2.data(), static_cast<int>(rank2.size()), SQLITE_TRANSIENT);

    int cnt = 0;
    if (sqlite3_step(st) == SQLITE_ROW) {
        cnt = sqlite3_column_int(st, 0);
    }

    sqlite3_finalize(st);
    return cnt;
}

bool shipHasAnyActiveCrew(sqlite3* db, std::int64_t shipId) {
    return countActiveCrew(db, shipId) > 0;
}

bool shipHasActiveCaptain(sqlite3* db, std::int64_t shipId) {
    return countActiveCrewWithRank2(db, shipId, kCaptainUa, kCaptainEn) > 0;
}

bool shipHasEngineer(sqlite3* db, std::int64_t shipId) {
    return countActiveCrewWithRank2(db, shipId, kEngineerUa, kEngineerEn) > 0;
}

bool shipHasResearcher(sqlite3* db, std::int64_t shipId) {
    return countActiveCrewWithRank2(db, shipId, kResearcherUa, kResearcherEn) > 0;
}

bool shipHasSoldier(sqlite3* db, std::int64_t shipId) {
    return countActiveCrewWithRank2(db, shipId, kSoldierUa, kSoldierEn) > 0;
}

// спеціальні вимоги по професії для типів кораблів
bool shipHasRequiredSpecialist(sqlite3* db, const Ship& ship) {
    if (ship.type == "research") {
        return shipHasResearcher(db, ship.id);
    }
    if (ship.type == "military") {
        return shipHasSoldier(db, ship.id);
    }
    if (ship.type == "cargo" || ship.type == "tanker") {
        return shipHasEngineer(db, ship.id);
    }
    return true;
}

bool canShipDepart(sqlite3* db, const Ship& ship, std::string& reasonOut) {
    if (!shipHasAnyActiveCrew(db, ship.id)) {
        reasonOut = "Корабель не має жодного активного екіпажу.";
        return false;
    }
    if (!shipHasActiveCaptain(db, ship.id)) {
        reasonOut = "На кораблі немає активного капітана.";
        return false;
    }
    if (!shipHasRequiredSpecialist(db, ship)) {
        reasonOut = "Для типу корабля немає необхідного спеціаліста.";
        return false;
    }
    return true;
}

} // namespace

// ================== LIST ==================

void ShipsController::list(const HttpRequestPtr&,
                           std::function<void(const HttpResponsePtr&)>&& cb) {
    try {
        ShipsRepo repo;
        const auto ships = repo.all();

        Json::Value arr(Json::arrayValue);
        for (const auto& s : ships) {
            arr.append(shipToJson(s));
        }

        cb(HttpResponse::newHttpJsonResponse(arr));
    } catch (const std::exception& ex) {
        LOG_ERROR << "ShipsController::list failed: " << ex.what();
        cb(jsonError("list failed", drogon::k500InternalServerError, ex.what()));
    }
}

// ================== CREATE ==================

void ShipsController::create(const HttpRequestPtr& req,
                             std::function<void(const HttpResponsePtr&)>&& cb) {
    const auto j = req->getJsonObject();
    if (!j) {
        cb(jsonError("json body required", drogon::k400BadRequest));
        return;
    }

    if (!(*j).isMember("name") || !(*j)["name"].isString() ||
        (*j)["name"].asString().empty()) {
        cb(jsonError("name is required", drogon::k400BadRequest));
        return;
    }

    Ship s;
    s.name       = (*j)["name"].asString();
    s.type       = (*j).isMember("type")       ? (*j)["type"].asString()      : "cargo";
    s.country    = (*j).isMember("country")    ? (*j)["country"].asString()   : "Unknown";
    s.port_id    = (*j).isMember("port_id")    ? (*j)["port_id"].asInt64()    : 0;
    s.status     = (*j).isMember("status")     ? (*j)["status"].asString()    : "docked";
    s.company_id = (*j).isMember("company_id") ? (*j)["company_id"].asInt64() : 0;

    if (!isValidStatus(s.status)) {
        cb(invalidStatusResponse(s.status));
        return;
    }

    try {
        ShipsRepo repo;
        const Ship created = repo.create(s);

        auto resp = HttpResponse::newHttpJsonResponse(shipToJson(created));
        resp->setStatusCode(drogon::k201Created);
        cb(resp);
    } catch (const std::exception& ex) {
        LOG_ERROR << "ShipsController::create failed name='" << s.name
                  << "': " << ex.what();
        cb(jsonError("failed to create", drogon::k500InternalServerError, ex.what()));
    }
}

// ================== GET ONE ==================

void ShipsController::getOne(const HttpRequestPtr&,
                             std::function<void(const HttpResponsePtr&)>&& cb,
                             std::int64_t id) {
    try {
        ShipsRepo repo;
        const auto s = repo.byId(id);
        if (!s) {
            cb(jsonError("not found", drogon::k404NotFound));
            return;
        }
        cb(HttpResponse::newHttpJsonResponse(shipToJson(*s)));
    } catch (const std::exception& ex) {
        LOG_ERROR << "ShipsController::getOne failed id=" << id
                  << ": " << ex.what();
        cb(jsonError("get failed", drogon::k500InternalServerError, ex.what()));
    }
}

// ================== UPDATE ==================

void ShipsController::updateOne(const HttpRequestPtr& req,
                                std::function<void(const HttpResponsePtr&)>&& cb,
                                std::int64_t id) {
    const auto j = req->getJsonObject();
    if (!j) {
        cb(jsonError("json body required", drogon::k400BadRequest));
        return;
    }

    try {
        ShipsRepo repo;
        const auto curOpt = repo.byId(id);
        if (!curOpt) {
            cb(jsonError("not found", drogon::k404NotFound));
            return;
        }

        Ship s = *curOpt;

        if ((*j).isMember("name"))       s.name       = (*j)["name"].asString();
        if ((*j).isMember("type"))       s.type       = (*j)["type"].asString();
        if ((*j).isMember("country"))    s.country    = (*j)["country"].asString();
        if ((*j).isMember("port_id"))    s.port_id    = (*j)["port_id"].asInt64();
        if ((*j).isMember("company_id")) s.company_id = (*j)["company_id"].asInt64();

        if ((*j).isMember("status")) {
            const std::string newStatus = (*j)["status"].asString();

            if (!isValidStatus(newStatus)) {
                cb(invalidStatusResponse(newStatus));
                return;
            }

            if (newStatus == "departed") {
                std::string reason;
                sqlite3* db = Db::instance().handle();

                if (!canShipDepart(db, s, reason)) {
                    Json::Value err;
                    err["error"]  = "Ship cannot depart";
                    err["reason"] = reason;

                    auto r = HttpResponse::newHttpJsonResponse(err);
                    r->setStatusCode(drogon::k409Conflict);
                    cb(r);
                    return;
                }
            }

            s.status = newStatus;
        }

        repo.update(s);

        Json::Value out;
        out["status"] = "updated";
        cb(HttpResponse::newHttpJsonResponse(out));
    } catch (const std::exception& ex) {
        LOG_ERROR << "ShipsController::updateOne failed id=" << id
                  << ": " << ex.what();
        cb(jsonError("failed to update", drogon::k500InternalServerError, ex.what()));
    }
}

// ================== DELETE ==================

void ShipsController::deleteOne(const HttpRequestPtr&,
                                std::function<void(const HttpResponsePtr&)>&& cb,
                                std::int64_t id) {
    try {
        ShipsRepo repo;
        repo.remove(id);

        auto r = HttpResponse::newHttpResponse();
        r->setStatusCode(drogon::k204NoContent);
        cb(r);
    } catch (const std::exception& ex) {
        LOG_ERROR << "ShipsController::deleteOne failed id=" << id
                  << ": " << ex.what();
        cb(jsonError("failed to delete", drogon::k500InternalServerError, ex.what()));
    }
}
