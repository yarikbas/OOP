#include <drogon/drogon.h>
#include <json/json.h>
#include "db/Db.h"
#include <iostream>

int main() {
    try {
        // ініціалізація БД + міграції
        Db::instance().runMigrations();
    } catch (const std::exception& e) {
        std::cerr << "[Db] init failed: " << e.what() << std::endl;
        return 3;
    }

    // health endpoint
    drogon::app().registerHandler(
        "/health",
        [](const drogon::HttpRequestPtr&,
           std::function<void (const drogon::HttpResponsePtr&)>&& cb) {

            Json::Value j;
            j["status"] = "ok";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(j);
            resp->setStatusCode(drogon::k200OK);
            cb(resp);
        },
        {drogon::Get}
    );

    drogon::app().loadConfigFile("config.json");
    drogon::app().run();

    return 0;
}