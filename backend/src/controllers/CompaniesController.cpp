#include "controllers/CompaniesController.h"
#include "repos/CompaniesRepo.h"
#include <json/json.h>
#include <drogon/drogon.h>  // для LOG_ERROR
using namespace drogon;

static HttpResponsePtr jerr(const std::string& msg, HttpStatusCode code) {
    Json::Value e;
    e["error"] = msg;
    auto r = HttpResponse::newHttpJsonResponse(e);
    r->setStatusCode(code);
    return r;
}

void CompaniesController::list(const HttpRequestPtr&, std::function<void(const HttpResponsePtr&)>&& cb) {
    CompaniesRepo repo;
    auto vec = repo.all();
    Json::Value arr(Json::arrayValue);
    for (auto &c : vec) {
        Json::Value j;
        j["id"]   = Json::Int64(c.id);
        j["name"] = c.name;
        arr.append(j);
    }
    cb(HttpResponse::newHttpJsonResponse(arr));
}

void CompaniesController::create(const HttpRequestPtr &req,
                                 std::function<void(const HttpResponsePtr &)> &&cb) {
    auto j = req->getJsonObject();
    if (!j || !(*j)["name"].isString() || (*j)["name"].asString().empty()) {
        cb(jerr("name required", k400BadRequest));
        return;
    }

    const auto name = (*j)["name"].asString();

    try {
        CompaniesRepo repo;
        auto c = repo.create(name);

        Json::Value out;
        out["id"]   = Json::Int64(c.id);
        out["name"] = c.name;

        auto resp = HttpResponse::newHttpJsonResponse(out);
        resp->setStatusCode(k201Created);  // 201 Created для успішного створення
        cb(resp);
    }
    catch (const std::exception &e) {
        // Логуємо реальну помилку з SQLite/Repo
        LOG_ERROR << "CompaniesController::create failed for name='" << name
                  << "': " << e.what();

        Json::Value err;
        err["error"]   = "create failed";
        err["details"] = e.what();  // дуже корисно на етапі налагодження

        auto resp = HttpResponse::newHttpJsonResponse(err);

        // Трошки розумніший вибір статус-коду (НЕ обов'язково, але зручно)
        std::string msg = e.what();
        if (msg.find("UNIQUE") != std::string::npos) {
            // Напр. UNIQUE constraint failed: companies.name
            resp->setStatusCode(k409Conflict);
        } else if (msg.find("NOT NULL") != std::string::npos) {
            // Напр. NOT NULL constraint failed: companies.some_column
            resp->setStatusCode(k400BadRequest);
        } else {
            resp->setStatusCode(k500InternalServerError);
        }

        cb(resp);
    }
    catch (...) {
        // На всякий випадок: якщо вилетить щось не std::exception
        LOG_ERROR << "CompaniesController::create failed with unknown exception for name='"
                  << name << "'";
        cb(jerr("create failed", k500InternalServerError));
    }
}

void CompaniesController::getOne(const HttpRequestPtr &,
                                 std::function<void(const HttpResponsePtr &)> &&cb,
                                 std::int64_t id) {
    CompaniesRepo repo;
    auto c = repo.byId(id);
    if (!c) {
        cb(jerr("not found", k404NotFound));
        return;
    }
    Json::Value out;
    out["id"]   = Json::Int64(c->id);
    out["name"] = c->name;
    cb(HttpResponse::newHttpJsonResponse(out));
}

void CompaniesController::update(const HttpRequestPtr &req,
                                 std::function<void(const HttpResponsePtr &)> &&cb,
                                 std::int64_t id) {
    auto j = req->getJsonObject();
    if (!j || !(*j)["name"].isString() || (*j)["name"].asString().empty()) {
        cb(jerr("name required", k400BadRequest));
        return;
    }

    CompaniesRepo repo;
    bool ok = repo.update(id, (*j)["name"].asString());
    if (!ok) {
        cb(jerr("not found or unchanged", k404NotFound));
        return;
    }

    Json::Value out;
    out["status"] = "updated";
    cb(HttpResponse::newHttpJsonResponse(out));
}

void CompaniesController::remove(const HttpRequestPtr &,
                                 std::function<void(const HttpResponsePtr &)> &&cb,
                                 std::int64_t id) {
    CompaniesRepo repo;
    bool ok = repo.remove(id);
    if (!ok) {
        cb(jerr("not found", k404NotFound));
        return;
    }
    auto r = HttpResponse::newHttpResponse();
    r->setStatusCode(k204NoContent);
    cb(r);
}

void CompaniesController::listPorts(const HttpRequestPtr &,
                                    std::function<void(const HttpResponsePtr &)> &&cb,
                                    std::int64_t id) {
    CompaniesRepo repo;
    auto vec = repo.ports(id);
    Json::Value arr(Json::arrayValue);
    for (auto &p : vec) {
        Json::Value j;
        j["id"]     = Json::Int64(p.id);
        j["name"]   = p.name;
        j["region"] = p.region;
        j["lat"]    = p.lat;
        j["lon"]    = p.lon;
        arr.append(j);
    }
    cb(HttpResponse::newHttpJsonResponse(arr));
}

void CompaniesController::addPort(const HttpRequestPtr &req,
                                  std::function<void(const HttpResponsePtr &)> &&cb,
                                  std::int64_t id) {
    auto j = req->getJsonObject();
    if (!j || !(*j)["port_id"].isIntegral()) {
        cb(jerr("port_id required", k400BadRequest));
        return;
    }

    bool isHq = (*j).isMember("is_hq") ? (*j)["is_hq"].asBool() : false;

    CompaniesRepo repo;
    bool ok = repo.addPort(id, (*j)["port_id"].asInt64(), isHq);
    if (!ok) {
        cb(jerr("already exists or invalid", k400BadRequest));
        return;
    }

    Json::Value out;
    out["status"] = "added";
    cb(HttpResponse::newHttpJsonResponse(out));
}

void CompaniesController::delPort(const HttpRequestPtr &,
                                  std::function<void(const HttpResponsePtr &)> &&cb,
                                  std::int64_t id,
                                  std::int64_t portId) {
    CompaniesRepo repo;
    bool ok = repo.removePort(id, portId);
    if (!ok) {
        cb(jerr("not found", k404NotFound));
        return;
    }
    auto r = HttpResponse::newHttpResponse();
    r->setStatusCode(k204NoContent);
    cb(r);
}

void CompaniesController::listShips(const HttpRequestPtr &,
                                    std::function<void(const HttpResponsePtr &)> &&cb,
                                    std::int64_t id) {
    CompaniesRepo repo;
    auto vec = repo.ships(id);
    Json::Value arr(Json::arrayValue);
    for (auto &s : vec) {
        Json::Value j;
        j["id"]         = Json::Int64(s.id);
        j["name"]       = s.name;
        j["type"]       = s.type;
        j["country"]    = s.country;
        j["port_id"]    = Json::Int64(s.port_id);
        j["status"]     = s.status;
        j["company_id"] = Json::Int64(s.company_id);
        arr.append(j);
    }
    cb(HttpResponse::newHttpJsonResponse(arr));
}
