// src/controllers/VoyageRecordsController.cpp
#include "controllers/VoyageRecordsController.h"
#include "repos/VoyageRecordsRepo.h"
#include <json/json.h>

using namespace drogon;

namespace {

Json::Value voyageRecordToJson(const VoyageRecord& v) {
    Json::Value j;
    j["id"] = Json::Int64(v.id);
    j["ship_id"] = Json::Int64(v.ship_id);
    j["from_port_id"] = Json::Int64(v.from_port_id);
    j["to_port_id"] = Json::Int64(v.to_port_id);
    j["departed_at"] = v.departed_at;
    j["arrived_at"] = v.arrived_at;
    j["actual_duration_hours"] = v.actual_duration_hours;
    j["planned_duration_hours"] = v.planned_duration_hours;
    j["distance_km"] = v.distance_km;
    j["fuel_consumed_tonnes"] = v.fuel_consumed_tonnes;
    j["total_cost_usd"] = v.total_cost_usd;
    j["total_revenue_usd"] = v.total_revenue_usd;
    j["cargo_list"] = v.cargo_list;
    j["crew_list"] = v.crew_list;
    j["weather_conditions"] = v.weather_conditions;
    j["notes"] = v.notes;
    return j;
}

VoyageRecord jsonToVoyageRecord(const Json::Value& j) {
    VoyageRecord v{};
    if (j.isMember("id")) v.id = j["id"].asInt64();
    if (j.isMember("ship_id")) v.ship_id = j["ship_id"].asInt64();
    if (j.isMember("from_port_id")) v.from_port_id = j["from_port_id"].asInt64();
    if (j.isMember("to_port_id")) v.to_port_id = j["to_port_id"].asInt64();
    if (j.isMember("departed_at")) v.departed_at = j["departed_at"].asString();
    if (j.isMember("arrived_at")) v.arrived_at = j["arrived_at"].asString();
    if (j.isMember("actual_duration_hours")) v.actual_duration_hours = j["actual_duration_hours"].asDouble();
    if (j.isMember("planned_duration_hours")) v.planned_duration_hours = j["planned_duration_hours"].asDouble();
    if (j.isMember("distance_km")) v.distance_km = j["distance_km"].asDouble();
    if (j.isMember("fuel_consumed_tonnes")) v.fuel_consumed_tonnes = j["fuel_consumed_tonnes"].asDouble();
    if (j.isMember("total_cost_usd")) v.total_cost_usd = j["total_cost_usd"].asDouble();
    if (j.isMember("total_revenue_usd")) v.total_revenue_usd = j["total_revenue_usd"].asDouble();
    if (j.isMember("cargo_list")) v.cargo_list = j["cargo_list"].asString();
    if (j.isMember("crew_list")) v.crew_list = j["crew_list"].asString();
    if (j.isMember("weather_conditions")) v.weather_conditions = j["weather_conditions"].asString();
    if (j.isMember("notes")) v.notes = j["notes"].asString();
    return v;
}

} // namespace

void VoyageRecordsController::getAll(const HttpRequestPtr& req,
                                     std::function<void(const HttpResponsePtr&)>&& callback) {
    try {
        VoyageRecordsRepo repo;
        auto items = repo.all();
        Json::Value arr(Json::arrayValue);
        for (const auto& item : items) {
            arr.append(voyageRecordToJson(item));
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

void VoyageRecordsController::getByShip(const HttpRequestPtr& req,
                                        std::function<void(const HttpResponsePtr&)>&& callback,
                                        int64_t shipId) {
    try {
        VoyageRecordsRepo repo;
        auto items = repo.byShipId(shipId);
        Json::Value arr(Json::arrayValue);
        for (const auto& item : items) {
            arr.append(voyageRecordToJson(item));
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

void VoyageRecordsController::getById(const HttpRequestPtr& req,
                                      std::function<void(const HttpResponsePtr&)>&& callback,
                                      int64_t id) {
    try {
        VoyageRecordsRepo repo;
        auto opt = repo.byId(id);
        if (!opt) {
            Json::Value err;
            err["error"] = "Voyage record not found";
            auto resp = HttpResponse::newHttpJsonResponse(err);
            resp->setStatusCode(k404NotFound);
            callback(resp);
            return;
        }
        auto resp = HttpResponse::newHttpJsonResponse(voyageRecordToJson(*opt));
        callback(resp);
    } catch (const std::exception& e) {
        Json::Value err;
        err["error"] = e.what();
        auto resp = HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}

void VoyageRecordsController::create(const HttpRequestPtr& req,
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
        
        auto record = jsonToVoyageRecord(*j);
        VoyageRecordsRepo repo;
        auto created = repo.create(record);
        auto resp = HttpResponse::newHttpJsonResponse(voyageRecordToJson(created));
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

void VoyageRecordsController::update(const HttpRequestPtr& req,
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
        
        auto record = jsonToVoyageRecord(*j);
        record.id = id;
        VoyageRecordsRepo repo;
        repo.update(record);
        auto resp = HttpResponse::newHttpJsonResponse(voyageRecordToJson(record));
        callback(resp);
    } catch (const std::exception& e) {
        Json::Value err;
        err["error"] = e.what();
        auto resp = HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}

void VoyageRecordsController::remove(const HttpRequestPtr& req,
                                     std::function<void(const HttpResponsePtr&)>&& callback,
                                     int64_t id) {
    try {
        VoyageRecordsRepo repo;
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
