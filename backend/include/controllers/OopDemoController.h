#pragma once
#include <drogon/HttpController.h>

class OopDemoController : public drogon::HttpController<OopDemoController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(OopDemoController::people, "/api/demo/oop/people", drogon::Get);
        ADD_METHOD_TO(OopDemoController::ships,  "/api/demo/oop/ships",  drogon::Get);
    METHOD_LIST_END

    void people(const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&&);
    void ships (const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&&);
};
