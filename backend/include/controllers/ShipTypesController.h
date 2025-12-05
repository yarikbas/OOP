#pragma once
#include <drogon/HttpController.h>
#include <cstdint>

class ShipTypesController : public drogon::HttpController<ShipTypesController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(ShipTypesController::list,      "/api/ship-types",     drogon::Get);
        ADD_METHOD_TO(ShipTypesController::create,    "/api/ship-types",     drogon::Post);
        ADD_METHOD_TO(ShipTypesController::getOne,    "/api/ship-types/{1}", drogon::Get);
        ADD_METHOD_TO(ShipTypesController::updateOne, "/api/ship-types/{1}", drogon::Put);
        ADD_METHOD_TO(ShipTypesController::deleteOne, "/api/ship-types/{1}", drogon::Delete);
    METHOD_LIST_END

    void list     (const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&&);
    void create   (const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&&);
    void getOne   (const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&&, std::int64_t id);
    void updateOne(const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&&, std::int64_t id);
    void deleteOne(const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&&, std::int64_t id);
};
