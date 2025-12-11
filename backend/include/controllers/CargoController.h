// include/controllers/CargoController.h
#pragma once

#include <drogon/HttpController.h>

class CargoController : public drogon::HttpController<CargoController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(CargoController::getAll, "/api/cargo", drogon::Get);
    ADD_METHOD_TO(CargoController::getByShip, "/api/cargo/ship/{1}", drogon::Get);
    ADD_METHOD_TO(CargoController::getByStatus, "/api/cargo/status/{1}", drogon::Get);
    ADD_METHOD_TO(CargoController::getById, "/api/cargo/{1}", drogon::Get);
    ADD_METHOD_TO(CargoController::create, "/api/cargo", drogon::Post);
    ADD_METHOD_TO(CargoController::update, "/api/cargo/{1}", drogon::Put);
    ADD_METHOD_TO(CargoController::remove, "/api/cargo/{1}", drogon::Delete);
    METHOD_LIST_END

    void getAll(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void getByShip(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   int64_t shipId);
    void getByStatus(const drogon::HttpRequestPtr& req,
                     std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                     const std::string& status);
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
