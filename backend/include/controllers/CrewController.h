#pragma once
#include <drogon/HttpController.h>

class CrewController : public drogon::HttpController<CrewController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(CrewController::listByShip,  "/api/ships/{1}/crew", drogon::Get);
        ADD_METHOD_TO(CrewController::assign,      "/api/crew/assign",    drogon::Post);
        ADD_METHOD_TO(CrewController::endByPerson, "/api/crew/end",       drogon::Post);
    METHOD_LIST_END

    void listByShip (const drogon::HttpRequestPtr&, std::function<void (const drogon::HttpResponsePtr &)> &&cb, long long shipId);
    void assign     (const drogon::HttpRequestPtr&, std::function<void (const drogon::HttpResponsePtr &)> &&cb);
    void endByPerson(const drogon::HttpRequestPtr&, std::function<void (const drogon::HttpResponsePtr &)> &&cb);
};
