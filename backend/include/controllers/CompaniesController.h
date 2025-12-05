#pragma once
#include <drogon/HttpController.h>
#include <cstdint>

class CompaniesController : public drogon::HttpController<CompaniesController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(CompaniesController::list,  "/api/companies", drogon::Get);
        ADD_METHOD_TO(CompaniesController::create,"/api/companies", drogon::Post);
        ADD_METHOD_TO(CompaniesController::getOne,"/api/companies/{1}", drogon::Get);
        ADD_METHOD_TO(CompaniesController::update,"/api/companies/{1}", drogon::Put);
        ADD_METHOD_TO(CompaniesController::remove,"/api/companies/{1}", drogon::Delete);

        ADD_METHOD_TO(CompaniesController::listPorts, "/api/companies/{1}/ports", drogon::Get);
        ADD_METHOD_TO(CompaniesController::addPort,   "/api/companies/{1}/ports", drogon::Post);
        ADD_METHOD_TO(CompaniesController::delPort,   "/api/companies/{1}/ports/{2}", drogon::Delete);

        ADD_METHOD_TO(CompaniesController::listShips, "/api/companies/{1}/ships", drogon::Get);
    METHOD_LIST_END

    void list   (const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&&);
    void create (const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&&);
    void getOne (const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&&, std::int64_t id);
    void update (const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&&, std::int64_t id);
    void remove (const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&&, std::int64_t id);

    void listPorts(const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&&, std::int64_t id);
    void addPort  (const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&&, std::int64_t id);
    void delPort  (const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&&, std::int64_t id, std::int64_t portId);

    void listShips(const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&&, std::int64_t id);
};
