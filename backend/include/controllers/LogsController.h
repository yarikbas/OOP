#pragma once

#include <drogon/HttpController.h>

class LogsController : public drogon::HttpController<LogsController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(LogsController::list,      "/api/logs",   drogon::Get);
        ADD_METHOD_TO(LogsController::exportData, "/api/export", drogon::Get);
        ADD_METHOD_TO(LogsController::exportCsv,  "/api/logs.csv", drogon::Get);
    METHOD_LIST_END

    void list(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb);
    void exportData(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb);
    void exportCsv(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& cb);
};
