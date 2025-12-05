#include "controllers/ShipsController.h"
#include "repos/ShipsRepo.h"
#include <json/json.h>

using namespace drogon;

static HttpResponsePtr jerr(const std::string& msg, HttpStatusCode code) {
    Json::Value e; e["error"] = msg;
    auto r = HttpResponse::newHttpJsonResponse(e);
    r->setStatusCode(code);
    return r;
}

void ShipsController::list(const HttpRequestPtr&,
                           std::function<void(const HttpResponsePtr&)>&& cb)
{
    ShipsRepo repo;
    auto ships = repo.all();
    Json::Value arr(Json::arrayValue);
    for (const auto& s : ships) {
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

void ShipsController::create(const HttpRequestPtr& req,
                             std::function<void(const HttpResponsePtr&)>&& cb)
{
    auto j = req->getJsonObject();
    if (!j || !(*j).isMember("name")) {
        cb(jerr("name is required", k400BadRequest));
        return;
    }

    Ship s;
    s.name       = (*j)["name"].asString();
    s.type       = (*j).isMember("type")       ? (*j)["type"].asString()      : "Cargo";
    s.country    = (*j).isMember("country")    ? (*j)["country"].asString()   : "Unknown";
    // якщо port_id не надано — ставимо 0, а репозиторій підбере перший валідний порт
    s.port_id    = (*j).isMember("port_id")    ? (*j)["port_id"].asInt64()    : 0;
    s.status     = (*j).isMember("status")     ? (*j)["status"].asString()    : "docked";
    s.company_id = (*j).isMember("company_id") ? (*j)["company_id"].asInt64() : 0;

    try {
        ShipsRepo repo;
        Ship created = repo.create(s);
        Json::Value out;
        out["id"]         = Json::Int64(created.id);
        out["name"]       = created.name;
        out["type"]       = created.type;
        out["country"]    = created.country;
        out["port_id"]    = Json::Int64(created.port_id);
        out["status"]     = created.status;
        out["company_id"] = Json::Int64(created.company_id);
        cb(HttpResponse::newHttpJsonResponse(out));
    } catch (const std::exception& ex) {
        cb(jerr(std::string("failed to create: ") + ex.what(), k500InternalServerError));
    } catch (...) {
        cb(jerr("failed to create", k500InternalServerError));
    }
}

void ShipsController::getOne(const HttpRequestPtr&,
                             std::function<void(const HttpResponsePtr&)>&& cb,
                             std::int64_t id)
{
    ShipsRepo repo;
    auto s = repo.byId(id);
    if (!s) { cb(jerr("not found", k404NotFound)); return; }

    Json::Value j;
    j["id"]         = Json::Int64(s->id);
    j["name"]       = s->name;
    j["type"]       = s->type;
    j["country"]    = s->country;
    j["port_id"]    = Json::Int64(s->port_id);
    j["status"]     = s->status;
    j["company_id"] = Json::Int64(s->company_id);
    cb(HttpResponse::newHttpJsonResponse(j));
}

void ShipsController::updateOne(const HttpRequestPtr& req,
                                std::function<void(const HttpResponsePtr&)>&& cb,
                                std::int64_t id)
{
    auto j = req->getJsonObject();
    if (!j) { cb(jerr("json body required", k400BadRequest)); return; }

    ShipsRepo repo;
    auto cur = repo.byId(id);
    if (!cur) { cb(jerr("not found", k404NotFound)); return; }

    Ship s = *cur;
    if ((*j).isMember("name"))       s.name       = (*j)["name"].asString();
    if ((*j).isMember("type"))       s.type       = (*j)["type"].asString();
    if ((*j).isMember("country"))    s.country    = (*j)["country"].asString();
    if ((*j).isMember("port_id"))    s.port_id    = (*j)["port_id"].asInt64();
    if ((*j).isMember("status"))     s.status     = (*j)["status"].asString();
    if ((*j).isMember("company_id")) s.company_id = (*j)["company_id"].asInt64();

    try {
        repo.update(s);
        Json::Value out; out["status"] = "updated";
        cb(HttpResponse::newHttpJsonResponse(out));
    } catch (const std::exception& ex) {
        cb(jerr(std::string("failed to update: ") + ex.what(), k500InternalServerError));
    } catch (...) {
        cb(jerr("failed to update", k500InternalServerError));
    }
}

void ShipsController::deleteOne(const HttpRequestPtr&,
                                std::function<void(const HttpResponsePtr&)>&& cb,
                                std::int64_t id)
{
    try {
        ShipsRepo repo;
        repo.remove(id);
        auto r = HttpResponse::newHttpResponse();
        r->setStatusCode(k204NoContent);
        cb(r);
    } catch (const std::exception& ex) {
        cb(jerr(std::string("failed to delete: ") + ex.what(), k500InternalServerError));
    } catch (...) {
        cb(jerr("failed to delete", k500InternalServerError));
    }
}
