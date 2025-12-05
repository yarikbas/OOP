#pragma once
#include <drogon/HttpController.h>
#include <cstdint>

class ShipsController : public drogon::HttpController<ShipsController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(ShipsController::list,     "/api/ships",       drogon::Get);
        ADD_METHOD_TO(ShipsController::create,   "/api/ships",       drogon::Post);
        ADD_METHOD_TO(ShipsController::getOne,   "/api/ships/{1}",   drogon::Get);
        ADD_METHOD_TO(ShipsController::updateOne,"/api/ships/{1}",   drogon::Put);
        ADD_METHOD_TO(ShipsController::deleteOne,"/api/ships/{1}",   drogon::Delete);
    METHOD_LIST_END

    void list    (const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&&);
    void create  (const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&&);
    void getOne  (const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&&, std::int64_t id);
    void updateOne(const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&&, std::int64_t id);
    void deleteOne(const drogon::HttpRequestPtr&, std::function<void(const drogon::HttpResponsePtr&)>&&, std::int64_t id);
};
