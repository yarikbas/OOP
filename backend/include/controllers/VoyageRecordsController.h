// include/controllers/VoyageRecordsController.h
#pragma once

#include <drogon/HttpController.h>

class VoyageRecordsController : public drogon::HttpController<VoyageRecordsController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(VoyageRecordsController::getAll, "/api/voyages", drogon::Get);
    ADD_METHOD_TO(VoyageRecordsController::getByShip, "/api/voyages/ship/{1}", drogon::Get);
    ADD_METHOD_TO(VoyageRecordsController::getById, "/api/voyages/{1}", drogon::Get);
    ADD_METHOD_TO(VoyageRecordsController::create, "/api/voyages", drogon::Post);
    ADD_METHOD_TO(VoyageRecordsController::update, "/api/voyages/{1}", drogon::Put);
    ADD_METHOD_TO(VoyageRecordsController::remove, "/api/voyages/{1}", drogon::Delete);
    METHOD_LIST_END

    void getAll(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    void getByShip(const drogon::HttpRequestPtr& req,
                   std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                   int64_t shipId);
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
