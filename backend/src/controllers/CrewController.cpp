// src/controllers/CrewController.cpp
#include "controllers/CrewController.h"
#include "repos/CrewRepo.h"

#include <drogon/drogon.h>
#include <json/json.h>

#include <cstdint>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

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

HttpResponsePtr jsonOk(const std::string& status) {
    Json::Value o;
    o["status"] = status;
    return HttpResponse::newHttpJsonResponse(o);
}

// ---------------- DB error -> HTTP ----------------

HttpStatusCode mapDbErrorToHttp(const std::string& msg) {
    if (msg.find("UNIQUE") != std::string::npos ||
        msg.find("unique") != std::string::npos) {
        return drogon::k409Conflict;
    }
    if (msg.find("FOREIGN KEY") != std::string::npos ||
        msg.find("foreign key") != std::string::npos) {
        // Тут можна і 404, але 409 теж ок для "ref integrity"
        return drogon::k409Conflict;
    }
    if (msg.find("NOT NULL") != std::string::npos ||
        msg.find("not null") != std::string::npos) {
        return drogon::k400BadRequest;
    }
    return drogon::k500InternalServerError;
}

// ---------------- Validation helpers ----------------

bool isIntegral(const Json::Value& v) {
    return v.isInt() || v.isUInt() || v.isInt64() || v.isUInt64();
}

bool readPositiveInt64(const Json::Value& j, const char* key, std::int64_t& out) {
    if (!j.isMember(key) || !isIntegral(j[key])) return false;
    out = j[key].asInt64();
    return out > 0;
}

// Якщо поле є — воно має бути string і не порожнє.
// Якщо поля нема — повертаємо false.
bool readNonEmptyStringIfPresent(const Json::Value& j, const char* key, std::string& out) {
    if (!j.isMember(key)) return false;
    if (!j[key].isString()) return false;
    out = j[key].asString();
    return !out.empty();
}

// ---------------- DTO -> JSON ----------------

Json::Value assignmentToJson(const CrewAssignment& a) {
    Json::Value j;
    j["id"]        = Json::Int64(a.id);
    j["person_id"] = Json::Int64(a.person_id);
    j["ship_id"]   = Json::Int64(a.ship_id);
    j["start_utc"] = a.start_utc;
    j["end_utc"]   = a.end_utc ? Json::Value(*a.end_utc)
                               : Json::Value(Json::nullValue);
    return j;
}

// ---------------- Time helper ----------------
// Thread-safe варіант для Windows/Linux.

std::string nowUtcIso() {
    std::time_t t = std::time(nullptr);

    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

} // namespace

// ================== LIST BY SHIP ==================

void CrewController::listByShip(const HttpRequestPtr&,
                                std::function<void(const HttpResponsePtr&)>&& cb,
                                long long shipId) {
    const auto sid = static_cast<std::int64_t>(shipId);
    if (sid <= 0) {
        cb(jsonError("shipId must be positive", drogon::k400BadRequest));
        return;
    }

    try {
        CrewRepo repo;
        const auto list = repo.currentCrewByShip(sid);

        Json::Value arr(Json::arrayValue);
        for (const auto& a : list) {
            arr.append(assignmentToJson(a));
        }

        cb(HttpResponse::newHttpJsonResponse(arr));
    } catch (const std::exception& e) {
        LOG_ERROR << "CrewController::listByShip failed shipId=" << shipId
                  << ": " << e.what();
        cb(jsonError("list crew failed", mapDbErrorToHttp(e.what()), e.what()));
    }
}

// ================== ASSIGN ==================

void CrewController::assign(const HttpRequestPtr& req,
                            std::function<void(const HttpResponsePtr&)>&& cb) {
    const auto j = req->getJsonObject();
    if (!j) {
        cb(jsonError("json body required", drogon::k400BadRequest));
        return;
    }

    std::int64_t personId = 0;
    std::int64_t shipId   = 0;

    if (!readPositiveInt64(*j, "person_id", personId) ||
        !readPositiveInt64(*j, "ship_id", shipId)) {
        cb(jsonError("person_id and ship_id must be positive integers",
                     drogon::k400BadRequest));
        return;
    }

    // start_utc:
    // - якщо передали — має бути валідним непорожнім string
    // - якщо не передали — ставимо nowUtcIso()
    std::string startUtc;
    if ((*j).isMember("start_utc")) {
        if (!readNonEmptyStringIfPresent(*j, "start_utc", startUtc)) {
            cb(jsonError("start_utc must be non-empty string",
                         drogon::k400BadRequest));
            return;
        }
    } else {
        startUtc = nowUtcIso();
    }

    try {
        CrewRepo repo;
        const auto created = repo.assign(personId, shipId, startUtc);

        if (!created) {
            // Бізнес-конфлікт:
            // 1 активне призначення на людину
            // 1 активне призначення на корабель
            cb(jsonError("assignment conflict",
                         drogon::k409Conflict,
                         "Person or ship already has an active assignment."));
            return;
        }

        auto resp = HttpResponse::newHttpJsonResponse(assignmentToJson(*created));
        resp->setStatusCode(drogon::k201Created);

        LOG_INFO << "CrewController::assign OK person_id=" << personId
                 << " ship_id=" << shipId << " id=" << created->id;

        cb(resp);
    } catch (const std::exception& e) {
        LOG_ERROR << "CrewController::assign exception person_id=" << personId
                  << " ship_id=" << shipId << ": " << e.what();
        cb(jsonError("assign failed", mapDbErrorToHttp(e.what()), e.what()));
    }
}

// ================== END BY PERSON ==================

void CrewController::endByPerson(const HttpRequestPtr& req,
                                 std::function<void(const HttpResponsePtr&)>&& cb) {
    const auto j = req->getJsonObject();
    if (!j) {
        cb(jsonError("json body required", drogon::k400BadRequest));
        return;
    }

    std::int64_t personId = 0;
    if (!readPositiveInt64(*j, "person_id", personId)) {
        cb(jsonError("person_id must be positive integer",
                     drogon::k400BadRequest));
        return;
    }

    // end_utc:
    // - якщо передали — має бути валідним непорожнім string
    // - якщо не передали — ставимо nowUtcIso()
    std::string endUtc;
    if ((*j).isMember("end_utc")) {
        if (!readNonEmptyStringIfPresent(*j, "end_utc", endUtc)) {
            cb(jsonError("end_utc must be non-empty string",
                         drogon::k400BadRequest));
            return;
        }
    } else {
        endUtc = nowUtcIso();
    }

    try {
        CrewRepo repo;
        const bool ok = repo.endActiveByPerson(personId, endUtc);

        if (!ok) {
            cb(jsonError("no active assignment", drogon::k404NotFound));
            return;
        }

        LOG_INFO << "CrewController::endByPerson OK person_id=" << personId
                 << " end_utc=" << endUtc;

        cb(jsonOk("ended"));
    } catch (const std::exception& e) {
        LOG_ERROR << "CrewController::endByPerson exception person_id=" << personId
                  << ": " << e.what();
        cb(jsonError("end assignment failed", mapDbErrorToHttp(e.what()), e.what()));
    }
}
