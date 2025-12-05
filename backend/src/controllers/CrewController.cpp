#include "controllers/CrewController.h"
#include "repos/CrewRepo.h"
#include <json/json.h>
#include <drogon/drogon.h>  // для LOG_*
using namespace drogon;

static HttpResponsePtr jerr(const std::string& msg, HttpStatusCode code) {
    Json::Value e;
    e["error"] = msg;
    auto r = HttpResponse::newHttpJsonResponse(e);
    r->setStatusCode(code);
    return r;
}

void CrewController::listByShip(const HttpRequestPtr&,
                                std::function<void(const HttpResponsePtr&)>&& cb,
                                long long shipId) {
    CrewRepo repo;
    auto list = repo.currentCrewByShip(shipId);

    Json::Value arr(Json::arrayValue);
    for (const auto& a : list) {
        Json::Value j;
        j["id"]        = Json::Int64(a.id);
        j["person_id"] = Json::Int64(a.person_id);
        j["ship_id"]   = Json::Int64(a.ship_id);
        j["start_utc"] = a.start_utc;
        if (a.end_utc.has_value())
            j["end_utc"] = *a.end_utc;
        else
            j["end_utc"] = Json::nullValue;
        arr.append(j);
    }

    cb(HttpResponse::newHttpJsonResponse(arr));
}

void CrewController::assign(const HttpRequestPtr& req,
                            std::function<void(const HttpResponsePtr&)>&& cb) {
    auto j = req->getJsonObject();
    if (!j || !(*j).isMember("person_id") || !(*j).isMember("ship_id")) {
        cb(jerr("person_id and ship_id are required", k400BadRequest));
        return;
    }

    long long personId = (*j)["person_id"].asInt64();
    long long shipId   = (*j)["ship_id"].asInt64();
    std::string start  = (*j).isMember("start_utc")
                           ? (*j)["start_utc"].asString()
                           : "2025-01-01T00:00:00Z";

    CrewRepo repo;
    auto created = repo.assign(personId, shipId, start);

    if (!created) {
        // Тут може бути або вже активне призначення, або помилка SQLite/prepare
        LOG_ERROR << "CrewController::assign failed for person_id=" << personId
                  << " ship_id=" << shipId
                  << " (maybe already active or DB error)";
        cb(jerr("failed to assign (maybe already active)", k409Conflict));
        return;
    }

    Json::Value out;
    out["id"]        = Json::Int64(created->id);
    out["person_id"] = Json::Int64(created->person_id);
    out["ship_id"]   = Json::Int64(created->ship_id);
    out["start_utc"] = created->start_utc;
    out["end_utc"]   = created->end_utc
                         ? Json::Value(*created->end_utc)
                         : Json::Value(Json::nullValue);

    LOG_INFO << "CrewController::assign OK person_id=" << personId
             << " ship_id=" << shipId << " id=" << created->id;

    cb(HttpResponse::newHttpJsonResponse(out));
}

void CrewController::endByPerson(const HttpRequestPtr& req,
                                 std::function<void(const HttpResponsePtr&)>&& cb) {
    auto j = req->getJsonObject();
    if (!j || !(*j).isMember("person_id") || !(*j).isMember("end_utc")) {
        cb(jerr("person_id and end_utc are required", k400BadRequest));
        return;
    }

    long long personId = (*j)["person_id"].asInt64();
    std::string end    = (*j)["end_utc"].asString();

    CrewRepo repo;
    bool ok = repo.endActiveByPerson(personId, end);

    if (!ok) {
        // Тут або немає активного призначення, або SQL не відпрацював
        LOG_ERROR << "CrewController::endByPerson: no active assignment or DB error "
                  << "for person_id=" << personId
                  << " end_utc=" << end;
        cb(jerr("no active assignment", k404NotFound));
        return;
    }

    LOG_INFO << "CrewController::endByPerson OK person_id=" << personId
             << " end_utc=" << end;

    Json::Value out;
    out["status"] = "ended";
    cb(HttpResponse::newHttpJsonResponse(out));
}
