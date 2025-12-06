#pragma once

#include <drogon/HttpController.h>
#include <cstdint>
#include <functional>

class PeopleController : public drogon::HttpController<PeopleController> {
public:
    using Callback = std::function<void(const drogon::HttpResponsePtr&)>;

    METHOD_LIST_BEGIN
        ADD_METHOD_TO(PeopleController::list,      "/api/people",     drogon::Get);
        ADD_METHOD_TO(PeopleController::create,    "/api/people",     drogon::Post);
        ADD_METHOD_TO(PeopleController::getOne,    "/api/people/{1}", drogon::Get);
        ADD_METHOD_TO(PeopleController::updateOne, "/api/people/{1}", drogon::Put);
        ADD_METHOD_TO(PeopleController::deleteOne, "/api/people/{1}", drogon::Delete);
    METHOD_LIST_END

    void list     (const drogon::HttpRequestPtr& req, Callback&& cb);
    void create   (const drogon::HttpRequestPtr& req, Callback&& cb);
    void getOne   (const drogon::HttpRequestPtr& req, Callback&& cb, std::int64_t id);
    void updateOne(const drogon::HttpRequestPtr& req, Callback&& cb, std::int64_t id);
    void deleteOne(const drogon::HttpRequestPtr& req, Callback&& cb, std::int64_t id);
};
