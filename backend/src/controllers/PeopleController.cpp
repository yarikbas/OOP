#include "controllers/PeopleController.h"
#include "repos/PeopleRepo.h"

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
        e["details"] = details; // на етапі налагодження корисно
    }
    auto r = HttpResponse::newHttpJsonResponse(e);
    r->setStatusCode(code);
    return r;
}

Json::Value personToJson(const Person& p) {
    Json::Value j;
    j["id"]        = Json::Int64(p.id);
    j["full_name"] = p.full_name;
    j["rank"]      = p.rank;
    j["active"]    = (p.active != 0);
    return j;
}

} // namespace

void PeopleController::list(const HttpRequestPtr&,
                            std::function<void(const HttpResponsePtr&)>&& cb) {
    try {
        PeopleRepo repo;
        const auto vec = repo.all();

        Json::Value arr(Json::arrayValue);
        for (const auto& p : vec) {
            arr.append(personToJson(p));
        }
        cb(HttpResponse::newHttpJsonResponse(arr));
    } catch (const std::exception& ex) {
        LOG_ERROR << "PeopleController::list failed: " << ex.what();
        cb(jsonError("list failed", drogon::k500InternalServerError, ex.what()));
    }
}

void PeopleController::create(const HttpRequestPtr& req,
                              std::function<void(const HttpResponsePtr&)>&& cb) {
    const auto j = req->getJsonObject();
    if (!j) {
        cb(jsonError("json body required", drogon::k400BadRequest));
        return;
    }

    if (!(*j).isMember("full_name") ||
        !(*j)["full_name"].isString() ||
        (*j)["full_name"].asString().empty()) {
        cb(jsonError("full_name is required", drogon::k400BadRequest));
        return;
    }

    Person p;
    p.full_name = (*j)["full_name"].asString();

    if ((*j).isMember("rank") && (*j)["rank"].isString()) {
        p.rank = (*j)["rank"].asString();
    } else {
        p.rank.clear();
    }

    if ((*j).isMember("active")) {
        p.active = (*j)["active"].asBool() ? 1 : 0;
    } else {
        p.active = 1;
    }

    try {
        PeopleRepo repo;
        const auto created = repo.create(p);

        auto resp = HttpResponse::newHttpJsonResponse(personToJson(created));
        resp->setStatusCode(drogon::k201Created);
        cb(resp);
    } catch (const std::exception& ex) {
        LOG_ERROR << "PeopleController::create failed full_name='"
                  << p.full_name << "': " << ex.what();
        cb(jsonError("create failed", drogon::k500InternalServerError, ex.what()));
    }
}

void PeopleController::getOne(const HttpRequestPtr&,
                              std::function<void(const HttpResponsePtr&)>&& cb,
                              std::int64_t id) {
    try {
        PeopleRepo repo;
        const auto p = repo.byId(id);
        if (!p) {
            cb(jsonError("not found", drogon::k404NotFound));
            return;
        }
        cb(HttpResponse::newHttpJsonResponse(personToJson(*p)));
    } catch (const std::exception& ex) {
        LOG_ERROR << "PeopleController::getOne failed id=" << id
                  << ": " << ex.what();
        cb(jsonError("get failed", drogon::k500InternalServerError, ex.what()));
    }
}

void PeopleController::updateOne(const HttpRequestPtr& req,
                                 std::function<void(const HttpResponsePtr&)>&& cb,
                                 std::int64_t id) {
    const auto j = req->getJsonObject();
    if (!j) {
        cb(jsonError("json body required", drogon::k400BadRequest));
        return;
    }

    try {
        PeopleRepo repo;
        const auto cur = repo.byId(id);
        if (!cur) {
            cb(jsonError("not found", drogon::k404NotFound));
            return;
        }

        Person p = *cur;

        if ((*j).isMember("full_name")) {
            if (!(*j)["full_name"].isString() || (*j)["full_name"].asString().empty()) {
                cb(jsonError("full_name must be non-empty string", drogon::k400BadRequest));
                return;
            }
            p.full_name = (*j)["full_name"].asString();
        }

        if ((*j).isMember("rank")) {
            if (!(*j)["rank"].isString()) {
                cb(jsonError("rank must be string", drogon::k400BadRequest));
                return;
            }
            p.rank = (*j)["rank"].asString();
        }

        if ((*j).isMember("active")) {
            p.active = (*j)["active"].asBool() ? 1 : 0;
        }

        repo.update(p);

        Json::Value ok;
        ok["status"] = "updated";
        cb(HttpResponse::newHttpJsonResponse(ok));
    } catch (const std::exception& ex) {
        LOG_ERROR << "PeopleController::updateOne failed id=" << id
                  << ": " << ex.what();
        cb(jsonError("update failed", drogon::k500InternalServerError, ex.what()));
    }
}

void PeopleController::deleteOne(const HttpRequestPtr&,
                                 std::function<void(const HttpResponsePtr&)>&& cb,
                                 std::int64_t id) {
    try {
        PeopleRepo repo;
        repo.remove(id);

        auto r = HttpResponse::newHttpResponse();
        r->setStatusCode(drogon::k204NoContent);
        cb(r);
    } catch (const std::exception& ex) {
        LOG_ERROR << "PeopleController::deleteOne failed id=" << id
                  << ": " << ex.what();
        cb(jsonError("delete failed", drogon::k500InternalServerError, ex.what()));
    }
}
