// src/controllers/ShipsController.cpp
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
#include <stdexcept>

namespace {

using drogon::HttpRequestPtr;
using drogon::HttpResponse;
using drogon::HttpResponsePtr;
using drogon::HttpStatusCode;

// ---------------- JSON helpers ----------------

HttpResponsePtr jsonError(const std::string& msg,
                          HttpStatusCode code,
                          const std::string& details = {}) {
    Json::Value e;
    e["error"] = msg;
    if (!details.empty()) {
        e["details"] = details;
    }
    auto r = HttpResponse::newHttpJsonResponse(e);
    r->setStatusCode(code);
    return r;
}

HttpResponsePtr jsonOk(const std::string& msg = "ok") {
    Json::Value o;
    o["status"] = msg;
    return HttpResponse::newHttpJsonResponse(o);
}

bool hasNonEmptyString(const Json::Value& j, const char* key) {
    return j.isMember(key) && j[key].isString() && !j[key].asString().empty();
}

bool isIntegral(const Json::Value& v) {
    return v.isInt() || v.isUInt() || v.isInt64() || v.isUInt64();
}

// ---------------- Ship JSON ----------------

Json::Value shipToJson(const Ship& s) {
    Json::Value j;
    j["id"]      = Json::Int64(s.id);
    j["name"]    = s.name;
    j["type"]    = s.type;
    j["country"] = s.country;
    j["status"]  = s.status;

    // якщо в БД NULL, parseShip дає 0 — віддаємо null у JSON
    j["port_id"] =
        (s.port_id > 0) ? Json::Value(Json::Int64(s.port_id))
                        : Json::Value(Json::nullValue);

    j["company_id"] =
        (s.company_id > 0) ? Json::Value(Json::Int64(s.company_id))
                           : Json::Value(Json::nullValue);

    return j;
}

// ---------------- Status rules ----------------

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

// ---------------- Ranks (UA + EN) ----------------

constexpr std::string_view kCaptainUa = "Капітан";
constexpr std::string_view kCaptainEn = "Captain";

// ---------------- SQL helpers for captain check ----------------

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

bool shipHasActiveCaptain(sqlite3* db, std::int64_t shipId) {
    return countActiveCrewWithRank2(db, shipId, kCaptainUa, kCaptainEn) > 0;
}

// ✅ ЄДИНЕ бізнес-правило для departed у твоєму проєкті
bool canShipDepart(sqlite3* db, const Ship& ship, std::string& reasonOut) {
    if (!shipHasActiveCaptain(db, ship.id)) {
        reasonOut = "На кораблі немає активного капітана.";
        return false;
    }
    return true;
}

// ---------------- Error mapping helper ----------------

HttpStatusCode mapDbErrorToHttp(const std::string& msg) {
    if (msg.find("UNIQUE") != std::string::npos ||
        msg.find("unique") != std::string::npos) {
        return drogon::k409Conflict;
    }
    if (msg.find("FOREIGN KEY") != std::string::npos ||
        msg.find("foreign key") != std::string::npos) {
        return drogon::k409Conflict;
    }
    if (msg.find("NOT NULL") != std::string::npos ||
        msg.find("not null") != std::string::npos) {
        return drogon::k400BadRequest;
    }
    return drogon::k500InternalServerError;
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

    const auto& body = *j;

    if (!hasNonEmptyString(body, "name")) {
        cb(jsonError("name is required", drogon::k400BadRequest));
        return;
    }

    if (body.isMember("type") && !body["type"].isString()) {
        cb(jsonError("type must be string", drogon::k400BadRequest));
        return;
    }
    if (body.isMember("country") && !body["country"].isString()) {
        cb(jsonError("country must be string", drogon::k400BadRequest));
        return;
    }
    if (body.isMember("status") && !body["status"].isString()) {
        cb(jsonError("status must be string", drogon::k400BadRequest));
        return;
    }

    if (body.isMember("port_id") && !body["port_id"].isNull() && !isIntegral(body["port_id"])) {
        cb(jsonError("port_id must be integer or null", drogon::k400BadRequest));
        return;
    }
    if (body.isMember("company_id") && !body["company_id"].isNull() && !isIntegral(body["company_id"])) {
        cb(jsonError("company_id must be integer or null", drogon::k400BadRequest));
        return;
    }

    Ship s;
    s.name       = body["name"].asString();
    s.type       = body.isMember("type")    ? body["type"].asString()    : "cargo";
    s.country    = body.isMember("country") ? body["country"].asString() : "Unknown";
    s.status     = body.isMember("status")  ? body["status"].asString()  : "docked";

    s.port_id    = (body.isMember("port_id") && !body["port_id"].isNull())
                   ? body["port_id"].asInt64()
                   : 0;

    s.company_id = (body.isMember("company_id") && !body["company_id"].isNull())
                   ? body["company_id"].asInt64()
                   : 0;

    if (s.port_id < 0 || s.company_id < 0) {
        cb(jsonError("port_id/company_id cannot be negative", drogon::k400BadRequest));
        return;
    }

    if (!isValidStatus(s.status)) {
        cb(invalidStatusResponse(s.status));
        return;
    }

    // ✅ не дозволяємо створювати departed напряму
    if (s.status == "departed") {
        cb(jsonError("cannot create ship with status 'departed'",
                     drogon::k409Conflict,
                     "Set status later via update with captain check."));
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

        const auto code = mapDbErrorToHttp(ex.what());
        cb(jsonError("failed to create", code, ex.what()));
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

    const auto& body = *j;

    try {
        ShipsRepo repo;
        const auto curOpt = repo.byId(id);
        if (!curOpt) {
            cb(jsonError("not found", drogon::k404NotFound));
            return;
        }

        Ship s = *curOpt;

        if (body.isMember("name")) {
            if (!body["name"].isString() || body["name"].asString().empty()) {
                cb(jsonError("name must be non-empty string", drogon::k400BadRequest));
                return;
            }
            s.name = body["name"].asString();
        }

        if (body.isMember("type")) {
            if (!body["type"].isString() || body["type"].asString().empty()) {
                cb(jsonError("type must be non-empty string", drogon::k400BadRequest));
                return;
            }
            s.type = body["type"].asString();
        }

        if (body.isMember("country")) {
            if (!body["country"].isString() || body["country"].asString().empty()) {
                cb(jsonError("country must be non-empty string", drogon::k400BadRequest));
                return;
            }
            s.country = body["country"].asString();
        }

        if (body.isMember("port_id")) {
            if (body["port_id"].isNull()) {
                s.port_id = 0;
            } else if (!isIntegral(body["port_id"])) {
                cb(jsonError("port_id must be integer or null", drogon::k400BadRequest));
                return;
            } else {
                s.port_id = body["port_id"].asInt64();
                if (s.port_id < 0) {
                    cb(jsonError("port_id cannot be negative", drogon::k400BadRequest));
                    return;
                }
            }
        }

        if (body.isMember("company_id")) {
            if (body["company_id"].isNull()) {
                s.company_id = 0;
            } else if (!isIntegral(body["company_id"])) {
                cb(jsonError("company_id must be integer or null", drogon::k400BadRequest));
                return;
            } else {
                s.company_id = body["company_id"].asInt64();
                if (s.company_id < 0) {
                    cb(jsonError("company_id cannot be negative", drogon::k400BadRequest));
                    return;
                }
            }
        }

        if (body.isMember("status")) {
            if (!body["status"].isString() || body["status"].asString().empty()) {
                cb(jsonError("status must be non-empty string", drogon::k400BadRequest));
                return;
            }

            const std::string newStatus = body["status"].asString();

            if (!isValidStatus(newStatus)) {
                cb(invalidStatusResponse(newStatus));
                return;
            }

            // ✅ ГОЛОВНЕ бізнес-правило
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

        cb(jsonOk("updated"));
    } catch (const std::exception& ex) {
        LOG_ERROR << "ShipsController::updateOne failed id=" << id
                  << ": " << ex.what();

        const auto code = mapDbErrorToHttp(ex.what());
        cb(jsonError("failed to update", code, ex.what()));
    }
}

// ================== DELETE ==================

void ShipsController::deleteOne(const HttpRequestPtr&,
                                std::function<void(const HttpResponsePtr&)>&& cb,
                                std::int64_t id) {
    try {
        ShipsRepo repo;

        const auto curOpt = repo.byId(id);
        if (!curOpt) {
            cb(jsonError("not found", drogon::k404NotFound));
            return;
        }

        repo.remove(id);

        auto r = HttpResponse::newHttpResponse();
        r->setStatusCode(drogon::k204NoContent);
        cb(r);
    } catch (const std::exception& ex) {
        LOG_ERROR << "ShipsController::deleteOne failed id=" << id
                  << ": " << ex.what();

        const auto code = mapDbErrorToHttp(ex.what());
        cb(jsonError("failed to delete", code, ex.what()));
    }
}
