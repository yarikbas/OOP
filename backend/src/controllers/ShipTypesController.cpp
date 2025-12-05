#include "controllers/ShipTypesController.h"
#include "repos/ShipTypesRepo.h"
#include <json/json.h>
using namespace drogon;

static HttpResponsePtr jerr(const std::string& msg, HttpStatusCode code) {
    Json::Value e; e["error"] = msg;
    auto r = HttpResponse::newHttpJsonResponse(e);
    r->setStatusCode(code);
    return r;
}

void ShipTypesController::list(const HttpRequestPtr&, std::function<void(const HttpResponsePtr&)>&& cb) {
    ShipTypesRepo repo; auto vec = repo.all();
    Json::Value arr(Json::arrayValue);
    for (const auto& t : vec) {
        Json::Value j;
        j["id"] = Json::Int64(t.id);
        j["code"] = t.code;
        j["name"] = t.name;
        j["description"] = t.description;
        arr.append(j);
    }
    cb(HttpResponse::newHttpJsonResponse(arr));
}

void ShipTypesController::create(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb) {
    auto j = req->getJsonObject();
    if (!j || !(*j).isMember("code") || !(*j).isMember("name"))
        { cb(jerr("code and name are required", k400BadRequest)); return; }
    ShipTypesRepo repo;
    ShipType t;
    t.code = (*j)["code"].asString();
    t.name = (*j)["name"].asString();
    t.description = (*j).isMember("description") ? (*j)["description"].asString() : "";
    try {
        auto created = repo.create(t);
        Json::Value out;
        out["id"] = Json::Int64(created.id);
        out["code"] = created.code;
        out["name"] = created.name;
        out["description"] = created.description;
        cb(HttpResponse::newHttpJsonResponse(out));
    } catch (const std::exception& ex) {
        cb(jerr(ex.what(), k500InternalServerError));
    }
}

void ShipTypesController::getOne(const HttpRequestPtr&, std::function<void(const HttpResponsePtr&)>&& cb, std::int64_t id) {
    ShipTypesRepo repo; auto t = repo.byId(id);
    if (!t) { cb(jerr("not found", k404NotFound)); return; }
    Json::Value j;
    j["id"] = Json::Int64(t->id);
    j["code"] = t->code;
    j["name"] = t->name;
    j["description"] = t->description;
    cb(HttpResponse::newHttpJsonResponse(j));
}

void ShipTypesController::updateOne(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::int64_t id) {
    auto j = req->getJsonObject(); if (!j) { cb(jerr("json body required", k400BadRequest)); return; }
    ShipTypesRepo repo; auto cur = repo.byId(id); if (!cur) { cb(jerr("not found", k404NotFound)); return; }
    ShipType t = *cur;
    if ((*j).isMember("code")) t.code = (*j)["code"].asString();
    if ((*j).isMember("name")) t.name = (*j)["name"].asString();
    if ((*j).isMember("description")) t.description = (*j)["description"].asString();
    try { repo.update(t); Json::Value ok; ok["status"]="updated"; cb(HttpResponse::newHttpJsonResponse(ok)); }
    catch (const std::exception& ex) { cb(jerr(ex.what(), k500InternalServerError)); }
}

void ShipTypesController::deleteOne(const HttpRequestPtr&, std::function<void(const HttpResponsePtr&)>&& cb, std::int64_t id) {
    ShipTypesRepo repo;
    try {
        repo.remove(id);
        auto r = HttpResponse::newHttpResponse(); r->setStatusCode(k204NoContent); cb(r);
    } catch (const std::exception& ex) { cb(jerr(ex.what(), k500InternalServerError)); }
}
