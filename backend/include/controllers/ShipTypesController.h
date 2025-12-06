#pragma once

#include <drogon/HttpController.h>
#include <cstdint>
#include <functional>

class ShipTypesController : public drogon::HttpController<ShipTypesController> {
public:
    using Callback = std::function<void(const drogon::HttpResponsePtr&)>;

    METHOD_LIST_BEGIN
        ADD_METHOD_TO(ShipTypesController::list,      "/api/ship-types",     drogon::Get);
        ADD_METHOD_TO(ShipTypesController::create,    "/api/ship-types",     drogon::Post);
        ADD_METHOD_TO(ShipTypesController::getOne,    "/api/ship-types/{1}", drogon::Get);
        ADD_METHOD_TO(ShipTypesController::updateOne, "/api/ship-types/{1}", drogon::Put);
        ADD_METHOD_TO(ShipTypesController::deleteOne, "/api/ship-types/{1}", drogon::Delete);
    METHOD_LIST_END

    void list     (const drogon::HttpRequestPtr& req, Callback&& cb);
    void create   (const drogon::HttpRequestPtr& req, Callback&& cb);
    void getOne   (const drogon::HttpRequestPtr& req, Callback&& cb, std::int64_t id);
    void updateOne(const drogon::HttpRequestPtr& req, Callback&& cb, std::int64_t id);
    void deleteOne(const drogon::HttpRequestPtr& req, Callback&& cb, std::int64_t id);
};
