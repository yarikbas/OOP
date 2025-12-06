#include "controllers/ShipTypesController.h"
#include "repos/ShipTypesRepo.h"

#include <drogon/drogon.h>
#include <json/json.h>

#include <cstdint>
#include <string>

namespace {

using drogon::HttpRequestPtr;
using drogon::HttpResponse;
using drogon::HttpResponsePtr;
using drogon::HttpStatusCode;

HttpResponsePtr jsonError(const std::string& msg,
                          HttpStatusCode code,
                          const std::string& details = {}) {
    Json::Value e;
    e["error"] = msg;
    if (!details.empty()) {
        e["details"] = details;
    }
    auto r = HttpResponse::newHttpJsonResponse(e);
    r->setStatusCode(code);
    return r;
}

Json::Value shipTypeToJson(const ShipType& t) {
    Json::Value j;
    j["id"]          = Json::Int64(t.id);
    j["code"]        = t.code;
    j["name"]        = t.name;
    j["description"] = t.description;
    return j;
}

bool nonEmptyStringMember(const Json::Value& v, const char* key) {
    return v.isMember(key) && v[key].isString() && !v[key].asString().empty();
}

} // namespace

void ShipTypesController::list(const HttpRequestPtr&,
                               std::function<void(const HttpResponsePtr&)>&& cb) {
    try {
        ShipTypesRepo repo;
        const auto vec = repo.all();

        Json::Value arr(Json::arrayValue);
        for (const auto& t : vec) {
            arr.append(shipTypeToJson(t));
        }
        cb(HttpResponse::newHttpJsonResponse(arr));
    } catch (const std::exception& ex) {
        LOG_ERROR << "ShipTypesController::list failed: " << ex.what();
        cb(jsonError("list failed", drogon::k500InternalServerError, ex.what()));
    }
}

void ShipTypesController::create(const HttpRequestPtr& req,
                                 std::function<void(const HttpResponsePtr&)>&& cb) {
    const auto j = req->getJsonObject();
    if (!j) {
        cb(jsonError("json body required", drogon::k400BadRequest));
        return;
    }

    if (!nonEmptyStringMember(*j, "code") || !nonEmptyStringMember(*j, "name")) {
        cb(jsonError("code and name are required", drogon::k400BadRequest));
        return;
    }

    ShipType t;
    t.code = (*j)["code"].asString();
    t.name = (*j)["name"].asString();

    if ((*j).isMember("description") && (*j)["description"].isString()) {
        t.description = (*j)["description"].asString();
    } else {
        t.description.clear();
    }

    try {
        ShipTypesRepo repo;
        const auto created = repo.create(t);

        auto resp = HttpResponse::newHttpJsonResponse(shipTypeToJson(created));
        resp->setStatusCode(drogon::k201Created);
        cb(resp);
    } catch (const std::exception& ex) {
        LOG_ERROR << "ShipTypesController::create failed code='"
                  << t.code << "': " << ex.what();
        cb(jsonError("create failed", drogon::k500InternalServerError, ex.what()));
    }
}

void ShipTypesController::getOne(const HttpRequestPtr&,
                                 std::function<void(const HttpResponsePtr&)>&& cb,
                                 std::int64_t id) {
    try {
        ShipTypesRepo repo;
        const auto t = repo.byId(id);

        if (!t) {
            cb(jsonError("not found", drogon::k404NotFound));
            return;
        }

        cb(HttpResponse::newHttpJsonResponse(shipTypeToJson(*t)));
    } catch (const std::exception& ex) {
        LOG_ERROR << "ShipTypesController::getOne failed id=" << id
                  << ": " << ex.what();
        cb(jsonError("get failed", drogon::k500InternalServerError, ex.what()));
    }
}

void ShipTypesController::updateOne(const HttpRequestPtr& req,
                                    std::function<void(const HttpResponsePtr&)>&& cb,
                                    std::int64_t id) {
    const auto j = req->getJsonObject();
    if (!j) {
        cb(jsonError("json body required", drogon::k400BadRequest));
        return;
    }

    try {
        ShipTypesRepo repo;
        const auto cur = repo.byId(id);

        if (!cur) {
            cb(jsonError("not found", drogon::k404NotFound));
            return;
        }

        ShipType t = *cur;

        if ((*j).isMember("code")) {
            if (!(*j)["code"].isString() || (*j)["code"].asString().empty()) {
                cb(jsonError("code must be non-empty string", drogon::k400BadRequest));
                return;
            }
            t.code = (*j)["code"].asString();
        }

        if ((*j).isMember("name")) {
            if (!(*j)["name"].isString() || (*j)["name"].asString().empty()) {
                cb(jsonError("name must be non-empty string", drogon::k400BadRequest));
                return;
            }
            t.name = (*j)["name"].asString();
        }

        if ((*j).isMember("description")) {
            if (!(*j)["description"].isString()) {
                cb(jsonError("description must be string", drogon::k400BadRequest));
                return;
            }
            t.description = (*j)["description"].asString();
        }

        repo.update(t);

        Json::Value ok;
        ok["status"] = "updated";
        cb(HttpResponse::newHttpJsonResponse(ok));
    } catch (const std::exception& ex) {
        LOG_ERROR << "ShipTypesController::updateOne failed id=" << id
                  << ": " << ex.what();
        cb(jsonError("update failed", drogon::k500InternalServerError, ex.what()));
    }
}

void ShipTypesController::deleteOne(const HttpRequestPtr&,
                                    std::function<void(const HttpResponsePtr&)>&& cb,
                                    std::int64_t id) {
    try {
        ShipTypesRepo repo;
        repo.remove(id);

        auto r = HttpResponse::newHttpResponse();
        r->setStatusCode(drogon::k204NoContent);
        cb(r);
    } catch (const std::exception& ex) {
        LOG_ERROR << "ShipTypesController::deleteOne failed id=" << id
                  << ": " << ex.what();
        cb(jsonError("delete failed", drogon::k500InternalServerError, ex.what()));
    }
}
