#pragma once

#include <drogon/HttpController.h>
#include <cstdint>
#include <functional>

class PortsController : public drogon::HttpController<PortsController> {
public:
    using Callback = std::function<void(const drogon::HttpResponsePtr&)>;

    METHOD_LIST_BEGIN
        ADD_METHOD_TO(PortsController::list,   "/api/ports",     drogon::Get);
        ADD_METHOD_TO(PortsController::create, "/api/ports",     drogon::Post);
        ADD_METHOD_TO(PortsController::getOne, "/api/ports/{1}", drogon::Get);
        ADD_METHOD_TO(PortsController::update, "/api/ports/{1}", drogon::Put);
        ADD_METHOD_TO(PortsController::remove, "/api/ports/{1}", drogon::Delete);
    METHOD_LIST_END

    void list  (const drogon::HttpRequestPtr& req, Callback&& cb);
    void create(const drogon::HttpRequestPtr& req, Callback&& cb);
    void getOne(const drogon::HttpRequestPtr& req, Callback&& cb, std::int64_t id);
    void update(const drogon::HttpRequestPtr& req, Callback&& cb, std::int64_t id);
    void remove(const drogon::HttpRequestPtr& req, Callback&& cb, std::int64_t id);
};
