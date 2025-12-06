#include <drogon/drogon.h>
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

            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k200OK);
            resp->setBody("OK");
            cb(resp);
        },
        {drogon::Get}
    );

    drogon::app().loadConfigFile("config.json");
    drogon::app().run();

    return 0;
}
