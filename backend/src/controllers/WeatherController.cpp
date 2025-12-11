// src/controllers/WeatherController.cpp
#include "controllers/WeatherController.h"
#include "repos/WeatherDataRepo.h"
#include <json/json.h>
#include <unordered_map>

using namespace drogon;

namespace {

Json::Value weatherDataToJson(const WeatherData& w) {
    Json::Value j;
    j["id"] = Json::Int64(w.id);
    j["port_id"] = Json::Int64(w.port_id);
    j["timestamp"] = w.timestamp;
    j["temperature_c"] = w.temperature_c;
    j["wind_speed_kmh"] = w.wind_speed_kmh;
    j["wind_direction_deg"] = w.wind_direction_deg;
    j["conditions"] = w.conditions;
    j["visibility_km"] = w.visibility_km;
    j["wave_height_m"] = w.wave_height_m;
    j["warnings"] = w.warnings;
    return j;
}

WeatherData jsonToWeatherData(const Json::Value& j) {
    WeatherData w{};
    if (j.isMember("id")) w.id = j["id"].asInt64();
    if (j.isMember("port_id")) w.port_id = j["port_id"].asInt64();
    if (j.isMember("timestamp")) w.timestamp = j["timestamp"].asString();
    if (j.isMember("temperature_c")) w.temperature_c = j["temperature_c"].asDouble();
    if (j.isMember("wind_speed_kmh")) w.wind_speed_kmh = j["wind_speed_kmh"].asDouble();
    if (j.isMember("wind_direction_deg")) w.wind_direction_deg = j["wind_direction_deg"].asInt();
    if (j.isMember("conditions")) w.conditions = j["conditions"].asString();
    if (j.isMember("visibility_km")) w.visibility_km = j["visibility_km"].asDouble();
    if (j.isMember("wave_height_m")) w.wave_height_m = j["wave_height_m"].asDouble();
    if (j.isMember("warnings")) w.warnings = j["warnings"].asString();
    return w;
}

} // namespace

void WeatherController::getAll(const HttpRequestPtr& req,
                                std::function<void(const HttpResponsePtr&)>&& callback) {
    try {
        WeatherDataRepo repo;
        auto items = repo.all();
        Json::Value arr(Json::arrayValue);
        for (const auto& item : items) {
            arr.append(weatherDataToJson(item));
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

void WeatherController::getByPort(const HttpRequestPtr& req,
                                  std::function<void(const HttpResponsePtr&)>&& callback,
                                  int64_t portId) {
    try {
        WeatherDataRepo repo;
        auto items = repo.byPortId(portId);
        Json::Value arr(Json::arrayValue);
        for (const auto& item : items) {
            arr.append(weatherDataToJson(item));
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

void WeatherController::getLatestAll(const HttpRequestPtr& req,
                                     std::function<void(const HttpResponsePtr&)>&& callback) {
    try {
        WeatherDataRepo repo;
        std::vector<WeatherData> list = repo.all();
        
        Json::Value arr(Json::arrayValue);
        for (size_t i = 0; i < list.size(); ++i) {
            arr.append(weatherDataToJson(list[i]));
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

void WeatherController::getById(const HttpRequestPtr& req,
                                std::function<void(const HttpResponsePtr&)>&& callback,
                                int64_t id) {
    try {
        WeatherDataRepo repo;
        auto opt = repo.byId(id);
        if (!opt) {
            Json::Value err;
            err["error"] = "Weather data not found";
            auto resp = HttpResponse::newHttpJsonResponse(err);
            resp->setStatusCode(k404NotFound);
            callback(resp);
            return;
        }
        auto resp = HttpResponse::newHttpJsonResponse(weatherDataToJson(*opt));
        callback(resp);
    } catch (const std::exception& e) {
        Json::Value err;
        err["error"] = e.what();
        auto resp = HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}

void WeatherController::create(const HttpRequestPtr& req,
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
        
        auto data = jsonToWeatherData(*j);
        WeatherDataRepo repo;
        auto created = repo.create(data);
        auto resp = HttpResponse::newHttpJsonResponse(weatherDataToJson(created));
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

void WeatherController::update(const HttpRequestPtr& req,
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
        
        auto data = jsonToWeatherData(*j);
        data.id = id;
        WeatherDataRepo repo;
        repo.update(data);
        auto resp = HttpResponse::newHttpJsonResponse(weatherDataToJson(data));
        callback(resp);
    } catch (const std::exception& e) {
        Json::Value err;
        err["error"] = e.what();
        auto resp = HttpResponse::newHttpJsonResponse(err);
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    }
}

void WeatherController::remove(const HttpRequestPtr& req,
                                std::function<void(const HttpResponsePtr&)>&& callback,
                                int64_t id) {
    try {
        WeatherDataRepo repo;
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
