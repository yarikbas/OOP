#include <drogon/drogon.h>
#include <json/json.h>
#include "db/Db.h"
#include <iostream>

// Forward declaration for auto-arrival timer
void setupAutoArrivalTimer();

int main() {
    try {
        // ініціалізація БД (всередині Db() вже є runMigrations)
        Db::instance();
    } catch (const std::exception& e) {
        std::cerr << "[Db] init failed: " << e.what() << std::endl;
        return 3;
    }

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

    // Try to locate config.json in several likely locations so running from
    // the build output directory still finds the project's config.
    std::string cfg = "config.json";
    try {
        namespace fs = std::filesystem;
        if (!fs::exists(cfg)) {
            const std::vector<std::string> cand = {"../config.json", "../../config.json"};
            for (const auto &c : cand) {
                if (fs::exists(c)) {
                    cfg = c;
                    break;
                }
            }
        }
    } catch (...) {
        // ignore filesystem errors and fall back to default
    }
    drogon::app().loadConfigFile(cfg);
    
    // Встановлюємо таймер для автоматичної обробки прибуттів кораблів
    setupAutoArrivalTimer();
    
    drogon::app().run();

    return 0;
}

/**
 * Функція налаштовує автоматичний таймер для перевірки прибуттів кораблів
 * Викликає endpoint /api/ships/process-arrivals кожні 60 секунд
 */
void setupAutoArrivalTimer() {
    LOG_INFO << "[AutoArrival] Starting auto-arrival timer (60 seconds interval)";
    
    auto timer_id = drogon::app().getLoop()->runEvery(60.0, []() {
        LOG_DEBUG << "[AutoArrival] Running scheduled ship arrival check";
        
        try {
            // Створюємо внутрішній HTTP клієнт
            auto client = drogon::HttpClient::newHttpClient("http://127.0.0.1:8082");
            auto req = drogon::HttpRequest::newHttpRequest();
            req->setMethod(drogon::Post);
            req->setPath("/api/ships/process-arrivals");
            
            client->sendRequest(req, [](drogon::ReqResult result, const drogon::HttpResponsePtr& response) {
                if (result == drogon::ReqResult::Ok && response) {
                    auto body = response->getBody();
                    LOG_INFO << "[AutoArrival] Response: " << body;
                } else {
                    LOG_WARN << "[AutoArrival] Failed to process arrivals: request failed";
                }
            });
        } catch (const std::exception& e) {
            LOG_ERROR << "[AutoArrival] Exception: " << e.what();
        }
    });
    
    LOG_INFO << "[AutoArrival] Timer started with ID: " << timer_id;
}