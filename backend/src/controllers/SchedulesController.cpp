// src/controllers/SchedulesController.cpp
#include "controllers/SchedulesController.h"
#include "repos/SchedulesRepo.h"
#include <json/json.h>

using namespace drogon;

namespace {

Json::Value scheduleToJson(const Schedule& s) {
    Json::Value j;
    j["id"] = Json::Int64(s.id);
    j["ship_id"] = Json::Int64(s.ship_id);
    j["route_name"] = s.route_name;
    j["from_port_id"] = Json::Int64(s.from_port_id);
    j["to_port_id"] = Json::Int64(s.to_port_id);
    j["departure_day_of_week"] = s.departure_day_of_week;
    j["departure_time"] = s.departure_time;
    j["recurring"] = s.recurring;
    j["is_active"] = s.is_active;
    j["notes"] = s.notes;
    return j;
}

Schedule jsonToSchedule(const Json::Value& j) {
    Schedule s{};
    if (j.isMember("id")) s.id = j["id"].asInt64();
    if (j.isMember("ship_id")) s.ship_id = j["ship_id"].asInt64();
    if (j.isMember("route_name")) s.route_name = j["route_name"].asString();
    if (j.isMember("from_port_id")) s.from_port_id = j["from_port_id"].asInt64();
    if (j.isMember("to_port_id")) s.to_port_id = j["to_port_id"].asInt64();
    if (j.isMember("departure_day_of_week")) s.departure_day_of_week = j["departure_day_of_week"].asInt();
    if (j.isMember("departure_time")) s.departure_time = j["departure_time"].asString();
    if (j.isMember("recurring")) s.recurring = j["recurring"].asString();
    if (j.isMember("is_active")) s.is_active = j["is_active"].asBool();
    if (j.isMember("notes")) s.notes = j["notes"].asString();
    return s;
}

} // namespace

void SchedulesController::getAll(const HttpRequestPtr& req,
                                 std::function<void(const HttpResponsePtr&)>&& callback) {
    try {
        SchedulesRepo repo;
        auto items = repo.all();
        Json::Value arr(Json::arrayValue);
        for (const auto& item : items) {
            arr.append(scheduleToJson(item));
        }
        auto resp = HttpResponse::newHttpJsonResponse(arr);
        callback(resp);
    } catch (const std::exception& e) {
        Json::Value err;
        err["error"] = e.what();
        auto resp = HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}

void SchedulesController::getByShip(const HttpRequestPtr& req,
                                    std::function<void(const HttpResponsePtr&)>&& callback,
                                    int64_t shipId) {
    try {
        SchedulesRepo repo;
        auto items = repo.byShipId(shipId);
        Json::Value arr(Json::arrayValue);
        for (const auto& item : items) {
            arr.append(scheduleToJson(item));
        }
        auto resp = HttpResponse::newHttpJsonResponse(arr);
        callback(resp);
    } catch (const std::exception& e) {
        Json::Value err;
        err["error"] = e.what();
        auto resp = HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}

void SchedulesController::getActive(const HttpRequestPtr& req,
                                    std::function<void(const HttpResponsePtr&)>&& callback) {
    try {
        SchedulesRepo repo;
        auto items = repo.active();
        Json::Value arr(Json::arrayValue);
        for (const auto& item : items) {
            arr.append(scheduleToJson(item));
        }
        auto resp = HttpResponse::newHttpJsonResponse(arr);
        callback(resp);
    } catch (const std::exception& e) {
        Json::Value err;
        err["error"] = e.what();
        auto resp = HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}

void SchedulesController::getById(const HttpRequestPtr& req,
                                  std::function<void(const HttpResponsePtr&)>&& callback,
                                  int64_t id) {
    try {
        SchedulesRepo repo;
        auto opt = repo.byId(id);
        if (!opt) {
            Json::Value err;
            err["error"] = "Schedule not found";
            auto resp = HttpResponse::newHttpJsonResponse(err);
            resp->setStatusCode(k404NotFound);
            callback(resp);
            return;
        }
        auto resp = HttpResponse::newHttpJsonResponse(scheduleToJson(*opt));
        callback(resp);
    } catch (const std::exception& e) {
        Json::Value err;
        err["error"] = e.what();
        auto resp = HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}

void SchedulesController::create(const HttpRequestPtr& req,
                                 std::function<void(const HttpResponsePtr&)>&& callback) {
    try {
        auto j = req->getJsonObject();
        if (!j) {
            Json::Value err;
            err["error"] = "Invalid JSON";
            auto resp = HttpResponse::newHttpJsonResponse(err);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }
        
        auto schedule = jsonToSchedule(*j);
        SchedulesRepo repo;
        auto created = repo.create(schedule);
        auto resp = HttpResponse::newHttpJsonResponse(scheduleToJson(created));
        resp->setStatusCode(k201Created);
        callback(resp);
    } catch (const std::exception& e) {
        Json::Value err;
        err["error"] = e.what();
        auto resp = HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}

void SchedulesController::update(const HttpRequestPtr& req,
                                 std::function<void(const HttpResponsePtr&)>&& callback,
                                 int64_t id) {
    try {
        auto j = req->getJsonObject();
        if (!j) {
            Json::Value err;
            err["error"] = "Invalid JSON";
            auto resp = HttpResponse::newHttpJsonResponse(err);
            resp->setStatusCode(k400BadRequest);
            callback(resp);
            return;
        }
        
        auto schedule = jsonToSchedule(*j);
        schedule.id = id;
        SchedulesRepo repo;
        repo.update(schedule);
        auto resp = HttpResponse::newHttpJsonResponse(scheduleToJson(schedule));
        callback(resp);
    } catch (const std::exception& e) {
        Json::Value err;
        err["error"] = e.what();
        auto resp = HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}

void SchedulesController::remove(const HttpRequestPtr& req,
                                 std::function<void(const HttpResponsePtr&)>&& callback,
                                 int64_t id) {
    try {
        SchedulesRepo repo;
        repo.remove(id);
        Json::Value result;
        result["success"] = true;
        auto resp = HttpResponse::newHttpJsonResponse(result);
        callback(resp);
    } catch (const std::exception& e) {
        Json::Value err;
        err["error"] = e.what();
        auto resp = HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}
