// include/controllers/SchedulesController.h
#pragma once

#include <drogon/HttpController.h>

class SchedulesController : public drogon::HttpController<SchedulesController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(SchedulesController::getAll, "/api/schedules", drogon::Get);
    ADD_METHOD_TO(SchedulesController::getByShip, "/api/schedules/ship/{1}", drogon::Get);
    ADD_METHOD_TO(SchedulesController::getActive, "/api/schedules/active", drogon::Get);
    ADD_METHOD_TO(SchedulesController::getById, "/api/schedules/{1}", drogon::Get);
    ADD_METHOD_TO(SchedulesController::create, "/api/schedules", drogon::Post);
    ADD_METHOD_TO(SchedulesController::update, "/api/schedules/{1}", drogon::Put);
    ADD_METHOD_TO(SchedulesController::remove, "/api/schedules/{1}", drogon::Delete);
    METHOD_LIST_END

    void getAll(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void getByShip(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   int64_t shipId);
    void getActive(const drogon::HttpRequestPtr& req,
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
