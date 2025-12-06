#include "controllers/ShipsController.h"
#include "repos/ShipsRepo.h"
#include "db/Db.h"
#include <json/json.h>
#include <sqlite3.h>
#include <algorithm>
#include <vector>
#include <string>

using namespace drogon;

// ------------ ХЕЛПЕР: JSON-помилка ------------
static HttpResponsePtr jerr(const std::string& msg, HttpStatusCode code) {
    Json::Value e;
    e["error"] = msg;
    auto r = HttpResponse::newHttpJsonResponse(e);
    r->setStatusCode(code);
    return r;
}

// ------------ ХЕЛПЕРИ ДЛЯ СТАТУСІВ КОРАБЛЯ ------------

static const std::vector<std::string> kShipStatuses = {
    "docked",      // докований
    "loading",     // завантажується
    "unloading",   // розвантажується
    "departed"     // відплив
};

static bool isValidStatus(const std::string& s) {
    return std::find(kShipStatuses.begin(), kShipStatuses.end(), s) != kShipStatuses.end();
}

// ------------ ХЕЛПЕРИ ДЛЯ ПЕРЕВІРКИ ЕКІПАЖУ ------------

static bool shipHasAnyActiveCrew(sqlite3* db, std::int64_t shipId) {
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(
            db,
            "SELECT COUNT(*) FROM crew_assignments "
            "WHERE ship_id = ? AND end_utc IS NULL;",
            -1, &st, nullptr) != SQLITE_OK) {
        if (st) sqlite3_finalize(st);
        return false;
    }

    sqlite3_bind_int64(st, 1, shipId);

    int cnt = 0;
    if (sqlite3_step(st) == SQLITE_ROW) {
        cnt = sqlite3_column_int(st, 0);
    }
    sqlite3_finalize(st);
    return cnt > 0;
}

static bool shipHasActiveCrewWithRank(sqlite3* db,
                                      std::int64_t shipId,
                                      const std::string& rank) {
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(
            db,
            "SELECT COUNT(*) "
            "FROM crew_assignments c "
            "JOIN people p ON p.id = c.person_id "
            "WHERE c.ship_id = ? "
            "  AND c.end_utc IS NULL "
            "  AND p.rank = ?;",
            -1, &st, nullptr) != SQLITE_OK) {
        if (st) sqlite3_finalize(st);
        return false;
    }

    sqlite3_bind_int64(st, 1, shipId);
    sqlite3_bind_text(st, 2, rank.c_str(), -1, SQLITE_TRANSIENT);

    int cnt = 0;
    if (sqlite3_step(st) == SQLITE_ROW) {
        cnt = sqlite3_column_int(st, 0);
    }
    sqlite3_finalize(st);
    return cnt > 0;
}

// спеціальні вимоги по професії для типів кораблів
static bool shipHasRequiredSpecialist(sqlite3* db, const Ship& ship) {
    // research -> Дослідник
    if (ship.type == "research") {
        return shipHasActiveCrewWithRank(db, ship.id, "Дослідник");
    }
    // military -> Солдат
    if (ship.type == "military") {
        return shipHasActiveCrewWithRank(db, ship.id, "Солдат");
    }
    // cargo/tanker -> Інженер (як приклад)
    if (ship.type == "cargo" || ship.type == "tanker") {
        return shipHasActiveCrewWithRank(db, ship.id, "Інженер");
    }
    // для інших типів додаткових вимог нема
    return true;
}

static bool canShipDepart(sqlite3* db, const Ship& ship, std::string& reasonOut) {
    if (!shipHasAnyActiveCrew(db, ship.id)) {
        reasonOut = "Корабель не має жодного активного екіпажу.";
        return false;
    }
    if (!shipHasActiveCrewWithRank(db, ship.id, "Капітан")) {
        reasonOut = "На кораблі немає активного капітана.";
        return false;
    }
    if (!shipHasRequiredSpecialist(db, ship)) {
        reasonOut = "Для типу корабля немає необхідного спеціаліста.";
        return false;
    }
    return true;
}

// ================== LIST ==================

void ShipsController::list(const HttpRequestPtr&,
                           std::function<void(const HttpResponsePtr&)>&& cb)
{
    ShipsRepo repo;
    auto ships = repo.all();
    Json::Value arr(Json::arrayValue);
    for (const auto& s : ships) {
        Json::Value j;
        j["id"]         = Json::Int64(s.id);
        j["name"]       = s.name;
        j["type"]       = s.type;
        j["country"]    = s.country;
        j["port_id"]    = Json::Int64(s.port_id);
        j["status"]     = s.status;
        j["company_id"] = Json::Int64(s.company_id);
        arr.append(j);
    }
    cb(HttpResponse::newHttpJsonResponse(arr));
}

// ================== CREATE ==================

void ShipsController::create(const HttpRequestPtr& req,
                             std::function<void(const HttpResponsePtr&)>&& cb)
{
    auto j = req->getJsonObject();
    if (!j || !(*j).isMember("name")) {
        cb(jerr("name is required", k400BadRequest));
        return;
    }

    Ship s;
    s.name       = (*j)["name"].asString();
    s.type       = (*j).isMember("type")       ? (*j)["type"].asString()      : "cargo";
    s.country    = (*j).isMember("country")    ? (*j)["country"].asString()   : "Unknown";
    s.port_id    = (*j).isMember("port_id")    ? (*j)["port_id"].asInt64()    : 0;
    s.status     = (*j).isMember("status")     ? (*j)["status"].asString()    : "docked";
    s.company_id = (*j).isMember("company_id") ? (*j)["company_id"].asInt64() : 0;

    // валідація статусу
    if (!isValidStatus(s.status)) {
        Json::Value err;
        err["error"] = "invalid status";
        err["status"] = s.status;
        Json::Value allowed(Json::arrayValue);
        for (const auto& st : kShipStatuses) allowed.append(st);
        err["allowed"] = allowed;

        auto r = HttpResponse::newHttpJsonResponse(err);
        r->setStatusCode(k400BadRequest);
        cb(r);
        return;
    }

    try {
        ShipsRepo repo;
        Ship created = repo.create(s);
        Json::Value out;
        out["id"]         = Json::Int64(created.id);
        out["name"]       = created.name;
        out["type"]       = created.type;
        out["country"]    = created.country;
        out["port_id"]    = Json::Int64(created.port_id);
        out["status"]     = created.status;
        out["company_id"] = Json::Int64(created.company_id);
        cb(HttpResponse::newHttpJsonResponse(out));
    } catch (const std::exception& ex) {
        cb(jerr(std::string("failed to create: ") + ex.what(), k500InternalServerError));
    } catch (...) {
        cb(jerr("failed to create", k500InternalServerError));
    }
}

// ================== GET ONE ==================

void ShipsController::getOne(const HttpRequestPtr&,
                             std::function<void(const HttpResponsePtr&)>&& cb,
                             std::int64_t id)
{
    ShipsRepo repo;
    auto s = repo.byId(id);
    if (!s) {
        cb(jerr("not found", k404NotFound));
        return;
    }

    Json::Value j;
    j["id"]         = Json::Int64(s->id);
    j["name"]       = s->name;
    j["type"]       = s->type;
    j["country"]    = s->country;
    j["port_id"]    = Json::Int64(s->port_id);
    j["status"]     = s->status;
    j["company_id"] = Json::Int64(s->company_id);
    cb(HttpResponse::newHttpJsonResponse(j));
}

// ================== UPDATE ==================

void ShipsController::updateOne(const HttpRequestPtr& req,
                                std::function<void(const HttpResponsePtr&)>&& cb,
                                std::int64_t id)
{
    auto j = req->getJsonObject();
    if (!j) {
        cb(jerr("json body required", k400BadRequest));
        return;
    }

    ShipsRepo repo;
    auto curOpt = repo.byId(id);
    if (!curOpt) {
        cb(jerr("not found", k404NotFound));
        return;
    }

    Ship s = *curOpt;

    // спочатку оновлюємо інші поля
    if ((*j).isMember("name"))       s.name       = (*j)["name"].asString();
    if ((*j).isMember("type"))       s.type       = (*j)["type"].asString();
    if ((*j).isMember("country"))    s.country    = (*j)["country"].asString();
    if ((*j).isMember("port_id"))    s.port_id    = (*j)["port_id"].asInt64();
    if ((*j).isMember("company_id")) s.company_id = (*j)["company_id"].asInt64();

    // окремо обробляємо статус
    if ((*j).isMember("status")) {
        const std::string newStatus = (*j)["status"].asString();

        if (!isValidStatus(newStatus)) {
            Json::Value err;
            err["error"] = "invalid status";
            err["status"] = newStatus;
            Json::Value allowed(Json::arrayValue);
            for (const auto& st : kShipStatuses) allowed.append(st);
            err["allowed"] = allowed;

            auto r = HttpResponse::newHttpJsonResponse(err);
            r->setStatusCode(k400BadRequest);
            cb(r);
            return;
        }

        // якщо хочемо перевести в departed — перевіряємо екіпаж / капітана / спеціаліста
        if (newStatus == "departed") {
            std::string reason;
            sqlite3* db = Db::instance().handle();
            if (!canShipDepart(db, s, reason)) {
                Json::Value err;
                err["error"]  = "Ship cannot depart";
                err["reason"] = reason;

                auto r = HttpResponse::newHttpJsonResponse(err);
                r->setStatusCode(k409Conflict); // конфлікт стану
                cb(r);
                return;
            }
        }

        s.status = newStatus;
    }

    try {
        repo.update(s);
        Json::Value out;
        out["status"] = "updated";
        cb(HttpResponse::newHttpJsonResponse(out));
    } catch (const std::exception& ex) {
        cb(jerr(std::string("failed to update: ") + ex.what(), k500InternalServerError));
    } catch (...) {
        cb(jerr("failed to update", k500InternalServerError));
    }
}

// ================== DELETE ==================

void ShipsController::deleteOne(const HttpRequestPtr&,
                                std::function<void(const HttpResponsePtr&)>&& cb,
                                std::int64_t id)
{
    try {
        ShipsRepo repo;
        repo.remove(id);
        auto r = HttpResponse::newHttpResponse();
        r->setStatusCode(k204NoContent);
        cb(r);
    } catch (const std::exception& ex) {
        cb(jerr(std::string("failed to delete: ") + ex.what(), k500InternalServerError));
    } catch (...) {
        cb(jerr("failed to delete", k500InternalServerError));
    }
}
