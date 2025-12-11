// include/controllers/WeatherController.h
#pragma once

#include <drogon/HttpController.h>

class WeatherController : public drogon::HttpController<WeatherController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(WeatherController::getAll, "/api/weather", drogon::Get);
    ADD_METHOD_TO(WeatherController::getByPort, "/api/weather/by-port/{1}", drogon::Get);
    ADD_METHOD_TO(WeatherController::getLatestAll, "/api/weather/latest", drogon::Get);
    ADD_METHOD_TO(WeatherController::getById, "/api/weather/{1}", drogon::Get);
    ADD_METHOD_TO(WeatherController::create, "/api/weather", drogon::Post);
    ADD_METHOD_TO(WeatherController::update, "/api/weather/{1}", drogon::Put);
    ADD_METHOD_TO(WeatherController::remove, "/api/weather/{1}", drogon::Delete);
    METHOD_LIST_END

    void getAll(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void getByPort(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   int64_t portId);
    void getLatestAll(const drogon::HttpRequestPtr& req,
                      std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void getById(const drogon::HttpRequestPtr& req,
                 std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                 int64_t id);
    void create(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void update(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                int64_t id);
    void remove(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                int64_t id);
};
