#include "controllers/PeopleController.h"
#include "repos/PeopleRepo.h"
#include <drogon/drogon.h>
#include <json/json.h>

using drogon::HttpRequestPtr;
using drogon::HttpResponse;
using drogon::HttpResponsePtr;
using drogon::HttpStatusCode;

namespace {

// --- Helpers ---

HttpResponsePtr jsonError(const std::string& msg, HttpStatusCode code) {
    Json::Value e;
    e["error"] = msg;
    auto r = HttpResponse::newHttpJsonResponse(e);
    r->setStatusCode(code);
    return r;
}

Json::Value personToJson(const Person& p) {
    Json::Value v;
    v["id"] = (Json::Int64)p.id;
    v["full_name"] = p.full_name;
    v["rank"] = p.rank;
    return v;
}

bool hasString(const Json::Value& j, const char* key) {
    return j.isMember(key) && j[key].isString() && !j[key].asString().empty();
}

} // namespace

// ================== LIST ==================
void PeopleController::list(const HttpRequestPtr&,
                            std::function<void(const HttpResponsePtr&)>&& cb) {
    try {
        PeopleRepo repo;
        auto all = repo.all();

        Json::Value arr(Json::arrayValue);
        for (const auto& p : all) {
            arr.append(personToJson(p));
        }
        cb(HttpResponse::newHttpJsonResponse(arr));
    }
    catch (const std::exception& e) {
        LOG_ERROR << "PeopleController::list error: " << e.what();
        cb(jsonError("Internal Error", drogon::k500InternalServerError));
    }
}

// ================== CREATE ==================
void PeopleController::create(const HttpRequestPtr& req,
                              std::function<void(const HttpResponsePtr&)>&& cb) {
    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        cb(jsonError("Invalid JSON", drogon::k400BadRequest));
        return;
    }
    const auto& j = *jsonPtr;

    if (!hasString(j, "full_name") || !hasString(j, "rank")) {
        cb(jsonError("Missing full_name or rank", drogon::k400BadRequest));
        return;
    }

    Person p;
    p.full_name = j["full_name"].asString();
    p.rank = j["rank"].asString();

    try {
        PeopleRepo repo;
        Person created = repo.create(p);
        
        auto resp = HttpResponse::newHttpJsonResponse(personToJson(created));
        resp->setStatusCode(drogon::k201Created);
        cb(resp);
    }
    catch (const std::exception& e) {
        LOG_ERROR << "PeopleController::create error: " << e.what();
        cb(jsonError("Failed to create person", drogon::k500InternalServerError));
    }
}

// ================== GET ONE ==================
void PeopleController::getOne(const HttpRequestPtr&,
                              std::function<void(const HttpResponsePtr&)>&& cb,
                              std::int64_t id) {
    try {
        PeopleRepo repo;
        auto pOpt = repo.byId(id);
        
        if (!pOpt) {
            cb(jsonError("Person not found", drogon::k404NotFound));
            return;
        }

        cb(HttpResponse::newHttpJsonResponse(personToJson(*pOpt)));
    }
    catch (const std::exception& e) {
        LOG_ERROR << "PeopleController::getOne error: " << e.what();
        cb(jsonError("Internal Error", drogon::k500InternalServerError));
    }
}

// ================== UPDATE ==================
void PeopleController::updateOne(const HttpRequestPtr& req,
                                 std::function<void(const HttpResponsePtr&)>&& cb,
                                 std::int64_t id) {
    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr) {
        cb(jsonError("Invalid JSON", drogon::k400BadRequest));
        return;
    }
    const auto& j = *jsonPtr;

    try {
        PeopleRepo repo;
        auto pOpt = repo.byId(id);
        if (!pOpt) {
            cb(jsonError("Person not found", drogon::k404NotFound));
            return;
        }

        Person p = *pOpt;
        if (hasString(j, "full_name")) p.full_name = j["full_name"].asString();
        if (hasString(j, "rank"))      p.rank      = j["rank"].asString();

        repo.update(p);
        
        // Повертаємо оновлений об'єкт
        cb(HttpResponse::newHttpJsonResponse(personToJson(p)));
    }
    catch (const std::exception& e) {
        LOG_ERROR << "PeopleController::update error: " << e.what();
        cb(jsonError("Failed to update", drogon::k500InternalServerError));
    }
}

// ================== DELETE ==================
void PeopleController::deleteOne(const HttpRequestPtr&,
                                 std::function<void(const HttpResponsePtr&)>&& cb,
                                 std::int64_t id) {
    try {
        PeopleRepo repo;
        // Перевіряємо, чи існує людина перед видаленням (опціонально)
        if (!repo.byId(id)) {
            cb(jsonError("Person not found", drogon::k404NotFound));
            return;
        }

        repo.remove(id);

        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k204NoContent);
        cb(resp);
    }
    catch (const std::exception& e) {
        LOG_ERROR << "PeopleController::delete error: " << e.what();
        cb(jsonError("Failed to delete", drogon::k500InternalServerError));
    }
}