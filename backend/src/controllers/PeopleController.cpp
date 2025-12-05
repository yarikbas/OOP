#include "controllers/PeopleController.h"
#include "repos/PeopleRepo.h"
#include <json/json.h>
using namespace drogon;

static HttpResponsePtr jerr(const std::string& msg, HttpStatusCode code) {
    Json::Value e; e["error"] = msg;
    auto r = HttpResponse::newHttpJsonResponse(e);
    r->setStatusCode(code);
    return r;
}

void PeopleController::list(const HttpRequestPtr&, std::function<void(const HttpResponsePtr&)>&& cb) {
    PeopleRepo repo; auto vec = repo.all();
    Json::Value arr(Json::arrayValue);
    for (const auto& p : vec) {
        Json::Value j;
        j["id"]        = Json::Int64(p.id);
        j["full_name"] = p.full_name;
        j["rank"]      = p.rank;
        j["active"]    = (p.active != 0);
        arr.append(j);
    }
    cb(HttpResponse::newHttpJsonResponse(arr));
}

void PeopleController::create(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb) {
    auto j = req->getJsonObject();
    if (!j || !(*j).isMember("full_name")) { cb(jerr("full_name is required", k400BadRequest)); return; }
    PeopleRepo repo;
    Person p;
    p.full_name = (*j)["full_name"].asString();
    p.rank      = (*j).isMember("rank")   ? (*j)["rank"].asString()   : "";
    p.active    = (*j).isMember("active") ? ((*j)["active"].asBool()?1:0) : 1;
    try {
        auto created = repo.create(p);
        Json::Value out;
        out["id"]        = Json::Int64(created.id);
        out["full_name"] = created.full_name;
        out["rank"]      = created.rank;
        out["active"]    = (created.active != 0);
        cb(HttpResponse::newHttpJsonResponse(out));
    } catch (const std::exception& ex) {
        cb(jerr(ex.what(), k500InternalServerError));
    }
}

void PeopleController::getOne(const HttpRequestPtr&, std::function<void(const HttpResponsePtr&)>&& cb, std::int64_t id) {
    PeopleRepo repo; auto p = repo.byId(id);
    if (!p) { cb(jerr("not found", k404NotFound)); return; }
    Json::Value j;
    j["id"]        = Json::Int64(p->id);
    j["full_name"] = p->full_name;
    j["rank"]      = p->rank;
    j["active"]    = (p->active != 0);
    cb(HttpResponse::newHttpJsonResponse(j));
}

void PeopleController::updateOne(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb, std::int64_t id) {
    auto j = req->getJsonObject();
    if (!j) { cb(jerr("json body required", k400BadRequest)); return; }
    PeopleRepo repo; auto cur = repo.byId(id);
    if (!cur) { cb(jerr("not found", k404NotFound)); return; }
    Person p = *cur;
    if ((*j).isMember("full_name")) p.full_name = (*j)["full_name"].asString();
    if ((*j).isMember("rank"))      p.rank      = (*j)["rank"].asString();
    if ((*j).isMember("active"))    p.active    = (*j)["active"].asBool()?1:0;
    try {
        repo.update(p);
        Json::Value ok; ok["status"] = "updated";
        cb(HttpResponse::newHttpJsonResponse(ok));
    } catch (const std::exception& ex) {
        cb(jerr(ex.what(), k500InternalServerError));
    }
}

void PeopleController::deleteOne(const HttpRequestPtr&, std::function<void(const HttpResponsePtr&)>&& cb, std::int64_t id) {
    PeopleRepo repo;
    try {
        repo.remove(id);
        auto r = HttpResponse::newHttpResponse();
        r->setStatusCode(k204NoContent);
        cb(r);
    } catch (const std::exception& ex) {
        cb(jerr(ex.what(), k500InternalServerError));
    }
}
