#include "controllers/PortsController.h"
#include "repos/PortsRepo.h"
#include "models/Port.h"
#include <json/json.h>

using namespace drogon;

static Json::Value portToJson(const Port& p) {
    Json::Value j;
    j["id"]     = Json::Int64(p.id);
    j["name"]   = p.name;
    j["region"] = p.region;
    j["lat"]    = p.lat;
    j["lon"]    = p.lon;
    return j;
}

void PortsController::list(const HttpRequestPtr&,
                           std::function<void(const HttpResponsePtr&)>&& cb) {
    PortsRepo repo;
    auto ports = repo.all();

    Json::Value arr(Json::arrayValue);
    for (const auto& p : ports) {
        arr.append(portToJson(p));
    }

    cb(HttpResponse::newHttpJsonResponse(arr));
}

void PortsController::create(const HttpRequestPtr& req,
                             std::function<void(const HttpResponsePtr&)>&& cb) {
    auto json = req->getJsonObject();
    if (!json) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("JSON body required");
        cb(resp);
        return;
    }

    Port p;
    p.name   = (*json)["name"].asString();
    p.region = (*json)["region"].asString();
    p.lat    = (*json)["lat"].asDouble();
    p.lon    = (*json)["lon"].asDouble();

    PortsRepo repo;
    auto created = repo.create(p);

    auto resp = HttpResponse::newHttpJsonResponse(portToJson(created));
    resp->setStatusCode(k201Created);
    cb(resp);
}

void PortsController::getOne(const HttpRequestPtr&,
                             std::function<void(const HttpResponsePtr&)>&& cb,
                             int64_t id) {
    PortsRepo repo;
    auto portOpt = repo.getById(id);
    if (!portOpt) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k404NotFound);
        resp->setBody("Port not found");
        cb(resp);
        return;
    }

    cb(HttpResponse::newHttpJsonResponse(portToJson(*portOpt)));
}

void PortsController::update(const HttpRequestPtr& req,
                             std::function<void(const HttpResponsePtr&)>&& cb,
                             int64_t id) {
    auto json = req->getJsonObject();
    if (!json) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k400BadRequest);
        resp->setBody("JSON body required");
        cb(resp);
        return;
    }

    PortsRepo repo;
    auto portOpt = repo.getById(id);
    if (!portOpt) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k404NotFound);
        resp->setBody("Port not found");
        cb(resp);
        return;
    }

    Port p = *portOpt;

    if ((*json).isMember("name"))   p.name   = (*json)["name"].asString();
    if ((*json).isMember("region")) p.region = (*json)["region"].asString();
    if ((*json).isMember("lat"))    p.lat    = (*json)["lat"].asDouble();
    if ((*json).isMember("lon"))    p.lon    = (*json)["lon"].asDouble();

    if (!repo.update(p)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k500InternalServerError);
        resp->setBody("Update failed");
        cb(resp);
        return;
    }

    cb(HttpResponse::newHttpJsonResponse(portToJson(p)));
}

void PortsController::remove(const HttpRequestPtr&,
                             std::function<void(const HttpResponsePtr&)>&& cb,
                             int64_t id) {
    PortsRepo repo;
    if (!repo.remove(id)) {
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k404NotFound);
        resp->setBody("Port not found");
        cb(resp);
        return;
    }

    auto resp = HttpResponse::newHttpResponse();
    resp->setStatusCode(k204NoContent);
    cb(resp);
}
