#pragma once

#include <drogon/HttpController.h>
#include <cstdint>
#include <functional>

class CrewController : public drogon::HttpController<CrewController> {
public:
    using Callback = std::function<void(const drogon::HttpResponsePtr&)>;

    METHOD_LIST_BEGIN
        ADD_METHOD_TO(CrewController::listByShip,  "/api/ships/{1}/crew", drogon::Get);
        ADD_METHOD_TO(CrewController::assign,      "/api/crew/assign",    drogon::Post);
        ADD_METHOD_TO(CrewController::endByPerson, "/api/crew/end",       drogon::Post);
    METHOD_LIST_END

    void listByShip (const drogon::HttpRequestPtr& req, Callback&& cb, std::int64_t shipId);
    void assign     (const drogon::HttpRequestPtr& req, Callback&& cb);
    void endByPerson(const drogon::HttpRequestPtr& req, Callback&& cb);
};
