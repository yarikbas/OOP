// src/controllers/CargoController.cpp
#include "controllers/CargoController.h"
#include "repos/CargoRepo.h"
#include <json/json.h>

using namespace drogon;

namespace {

Json::Value cargoToJson(const Cargo& c) {
    Json::Value j;
    j["id"] = Json::Int64(c.id);
    j["name"] = c.name;
    j["type"] = c.type;
    j["weight_tonnes"] = c.weight_tonnes;
    j["volume_m3"] = c.volume_m3;
    j["value_usd"] = c.value_usd;
    j["origin_port_id"] = Json::Int64(c.origin_port_id);
    j["destination_port_id"] = Json::Int64(c.destination_port_id);
    j["status"] = c.status;
    j["ship_id"] = Json::Int64(c.ship_id);
    j["loaded_at"] = c.loaded_at;
    j["delivered_at"] = c.delivered_at;
    j["notes"] = c.notes;
    return j;
}

Cargo jsonToCargo(const Json::Value& j) {
    Cargo c{};
    if (j.isMember("id")) c.id = j["id"].asInt64();
    if (j.isMember("name")) c.name = j["name"].asString();
    if (j.isMember("type")) c.type = j["type"].asString();
    if (j.isMember("weight_tonnes")) c.weight_tonnes = j["weight_tonnes"].asDouble();
    if (j.isMember("volume_m3")) c.volume_m3 = j["volume_m3"].asDouble();
    if (j.isMember("value_usd")) c.value_usd = j["value_usd"].asDouble();
    if (j.isMember("origin_port_id")) c.origin_port_id = j["origin_port_id"].asInt64();
    if (j.isMember("destination_port_id")) c.destination_port_id = j["destination_port_id"].asInt64();
    if (j.isMember("status")) c.status = j["status"].asString();
    if (j.isMember("ship_id")) c.ship_id = j["ship_id"].asInt64();
    if (j.isMember("loaded_at")) c.loaded_at = j["loaded_at"].asString();
    if (j.isMember("delivered_at")) c.delivered_at = j["delivered_at"].asString();
    if (j.isMember("notes")) c.notes = j["notes"].asString();
    return c;
}

} // namespace

void CargoController::getAll(const HttpRequestPtr& req,
                              std::function<void(const HttpResponsePtr&)>&& callback) {
    try {
        CargoRepo repo;
        auto items = repo.all();
        Json::Value arr(Json::arrayValue);
        for (const auto& item : items) {
            arr.append(cargoToJson(item));
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

void CargoController::getByShip(const HttpRequestPtr& req,
                                std::function<void(const HttpResponsePtr&)>&& callback,
                                int64_t shipId) {
    try {
        CargoRepo repo;
        auto items = repo.byShipId(shipId);
        Json::Value arr(Json::arrayValue);
        for (const auto& item : items) {
            arr.append(cargoToJson(item));
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

void CargoController::getByStatus(const HttpRequestPtr& req,
                                   std::function<void(const HttpResponsePtr&)>&& callback,
                                   const std::string& status) {
    try {
        CargoRepo repo;
        auto items = repo.byStatus(status);
        Json::Value arr(Json::arrayValue);
        for (const auto& item : items) {
            arr.append(cargoToJson(item));
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

void CargoController::getById(const HttpRequestPtr& req,
                               std::function<void(const HttpResponsePtr&)>&& callback,
                               int64_t id) {
    try {
        CargoRepo repo;
        auto opt = repo.byId(id);
        if (!opt) {
            Json::Value err;
            err["error"] = "Cargo not found";
            auto resp = HttpResponse::newHttpJsonResponse(err);
            resp->setStatusCode(k404NotFound);
            callback(resp);
            return;
        }
        auto resp = HttpResponse::newHttpJsonResponse(cargoToJson(*opt));
        callback(resp);
    } catch (const std::exception& e) {
        Json::Value err;
        err["error"] = e.what();
        auto resp = HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}

void CargoController::create(const HttpRequestPtr& req,
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
        
        auto cargo = jsonToCargo(*j);
        CargoRepo repo;
        auto created = repo.create(cargo);
        auto resp = HttpResponse::newHttpJsonResponse(cargoToJson(created));
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

void CargoController::update(const HttpRequestPtr& req,
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
        
        auto cargo = jsonToCargo(*j);
        cargo.id = id;
        CargoRepo repo;
        repo.update(cargo);
        auto resp = HttpResponse::newHttpJsonResponse(cargoToJson(cargo));
        callback(resp);
    } catch (const std::exception& e) {
        Json::Value err;
        err["error"] = e.what();
        auto resp = HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}

void CargoController::remove(const HttpRequestPtr& req,
                              std::function<void(const HttpResponsePtr&)>&& callback,
                              int64_t id) {
    try {
        CargoRepo repo;
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
