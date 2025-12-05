#pragma once
#include <drogon/HttpController.h>

class PortsController : public drogon::HttpController<PortsController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(PortsController::list, "/api/ports", drogon::Get);
    METHOD_LIST_END

    void list(const drogon::HttpRequestPtr&,
              std::function<void(const drogon::HttpResponsePtr&)>&&);
};
