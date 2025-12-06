#include "controllers/CrewController.h"
#include "repos/CrewRepo.h"

#include <drogon/drogon.h>
#include <json/json.h>

#include <cstdint>

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
        e["details"] = details; // корисно при дебазі
    }
    auto r = HttpResponse::newHttpJsonResponse(e);
    r->setStatusCode(code);
    return r;
}

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

// Залишаємо твоє дефолтне значення, але принаймні
// робимо його явним константним "плейсхолдером".
constexpr const char* kDefaultStartUtc = "2025-01-01T00:00:00Z";

} // namespace

void CrewController::listByShip(const HttpRequestPtr&,
                                std::function<void(const HttpResponsePtr&)>&& cb,
                                long long shipId) {
    try {
        CrewRepo repo;
        const auto list = repo.currentCrewByShip(static_cast<std::int64_t>(shipId));

        Json::Value arr(Json::arrayValue);
        for (const auto& a : list) {
            arr.append(assignmentToJson(a));
        }

        cb(HttpResponse::newHttpJsonResponse(arr));
    } catch (const std::exception& e) {
        LOG_ERROR << "CrewController::listByShip failed shipId=" << shipId
                  << ": " << e.what();
        cb(jsonError("list crew failed", drogon::k500InternalServerError, e.what()));
    }
}

void CrewController::assign(const HttpRequestPtr& req,
                            std::function<void(const HttpResponsePtr&)>&& cb) {
    const auto j = req->getJsonObject();
    if (!j) {
        cb(jsonError("json body required", drogon::k400BadRequest));
        return;
    }

    if (!(*j).isMember("person_id") || !(*j)["person_id"].isIntegral() ||
        !(*j).isMember("ship_id")   || !(*j)["ship_id"].isIntegral()) {
        cb(jsonError("person_id and ship_id are required", drogon::k400BadRequest));
        return;
    }

    const std::int64_t personId = (*j)["person_id"].asInt64();
    const std::int64_t shipId   = (*j)["ship_id"].asInt64();

    const std::string start =
        (*j).isMember("start_utc") && (*j)["start_utc"].isString()
            ? (*j)["start_utc"].asString()
            : kDefaultStartUtc;

    try {
        CrewRepo repo;
        const auto created = repo.assign(personId, shipId, start);

        if (!created) {
            // Це може бути бізнес-конфлікт (вже активне),
            // або невдала операція репозиторію.
            LOG_WARN << "CrewController::assign failed person_id=" << personId
                     << " ship_id=" << shipId
                     << " (maybe already active)";
            cb(jsonError("failed to assign (maybe already active)", drogon::k409Conflict));
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
        cb(jsonError("assign failed", drogon::k500InternalServerError, e.what()));
    }
}

void CrewController::endByPerson(const HttpRequestPtr& req,
                                 std::function<void(const HttpResponsePtr&)>&& cb) {
    const auto j = req->getJsonObject();
    if (!j) {
        cb(jsonError("json body required", drogon::k400BadRequest));
        return;
    }

    if (!(*j).isMember("person_id") || !(*j)["person_id"].isIntegral() ||
        !(*j).isMember("end_utc")   || !(*j)["end_utc"].isString() ||
        (*j)["end_utc"].asString().empty()) {
        cb(jsonError("person_id and end_utc are required", drogon::k400BadRequest));
        return;
    }

    const std::int64_t personId = (*j)["person_id"].asInt64();
    const std::string end       = (*j)["end_utc"].asString();

    try {
        CrewRepo repo;
        const bool ok = repo.endActiveByPerson(personId, end);

        if (!ok) {
            LOG_WARN << "CrewController::endByPerson: no active assignment "
                     << "for person_id=" << personId
                     << " end_utc=" << end;
            cb(jsonError("no active assignment", drogon::k404NotFound));
            return;
        }

        LOG_INFO << "CrewController::endByPerson OK person_id=" << personId
                 << " end_utc=" << end;

        Json::Value out;
        out["status"] = "ended";
        cb(HttpResponse::newHttpJsonResponse(out));
    } catch (const std::exception& e) {
        LOG_ERROR << "CrewController::endByPerson exception person_id=" << personId
                  << ": " << e.what();
        cb(jsonError("end assignment failed", drogon::k500InternalServerError, e.what()));
    }
}
