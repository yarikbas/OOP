#include <drogon/drogon.h>
#include "db/Db.h"
using namespace drogon;

int main() {
    // Ініціалізуємо БД (створить схему та посіє порти за потреби)
    Db::instance();

    // Завантажуємо конфіг (порт 8081, шляхи logs/uploads, рівень логів тощо)
    app().loadConfigFile("config.json");

    // /health
    app().registerHandler("/health",
        [](const HttpRequestPtr&, std::function<void(const HttpResponsePtr&)>&& cb){
            auto r = HttpResponse::newHttpResponse();
            r->setContentTypeCode(CT_APPLICATION_JSON);
            r->setBody(R"({"status":"ok"})");
            cb(r);
        }, {Get});

    app().run();
}
