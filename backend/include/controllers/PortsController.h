#pragma once
#include <drogon/HttpController.h>

class PortsController : public drogon::HttpController<PortsController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(PortsController::list,   "/api/ports",        drogon::Get);
        ADD_METHOD_TO(PortsController::create, "/api/ports",        drogon::Post);
        ADD_METHOD_TO(PortsController::getOne, "/api/ports/{1}",    drogon::Get);
        ADD_METHOD_TO(PortsController::update, "/api/ports/{1}",    drogon::Put);
        ADD_METHOD_TO(PortsController::remove, "/api/ports/{1}",    drogon::Delete);
    METHOD_LIST_END

    void list  (const drogon::HttpRequestPtr&,
                std::function<void(const drogon::HttpResponsePtr&)>&&);

    void create(const drogon::HttpRequestPtr&,
                std::function<void(const drogon::HttpResponsePtr&)>&&);

    void getOne(const drogon::HttpRequestPtr&,
                std::function<void(const drogon::HttpResponsePtr&)>&&, int64_t id);

    void update(const drogon::HttpRequestPtr&,
                std::function<void(const drogon::HttpResponsePtr&)>&&, int64_t id);

    void remove(const drogon::HttpRequestPtr&,
                std::function<void(const drogon::HttpResponsePtr&)>&&, int64_t id);
};
