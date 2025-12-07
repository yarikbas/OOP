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

// ---------------- JSON helpers ----------------

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

HttpResponsePtr jsonOk(const std::string& status = "ok") {
    Json::Value o;
    o["status"] = status;
    return HttpResponse::newHttpJsonResponse(o);
}

// best-effort мапінг типових SQLite повідомлень
HttpStatusCode mapDbErrorToHttp(const std::string& msg) {
    if (msg.find("UNIQUE") != std::string::npos || msg.find("unique") != std::string::npos) {
        return drogon::k409Conflict;
    }
    if (msg.find("FOREIGN KEY") != std::string::npos || msg.find("foreign key") != std::string::npos) {
        return drogon::k409Conflict;
    }
    if (msg.find("NOT NULL") != std::string::npos || msg.find("not null") != std::string::npos) {
        return drogon::k400BadRequest;
    }
    return drogon::k500InternalServerError;
}

// ---------------- JSON mapping ----------------

Json::Value shipTypeToJson(const ShipType& t) {
    Json::Value j;
    j["id"]          = Json::Int64(t.id);
    j["code"]        = t.code;
    j["name"]        = t.name;
    j["description"] = t.description;
    return j;
}

// ---------------- Validation helpers ----------------

bool hasNonEmptyString(const Json::Value& v, const char* key) {
    return v.isMember(key) && v[key].isString() && !v[key].asString().empty();
}

bool isStringOrNull(const Json::Value& v, const char* key) {
    if (!v.isMember(key)) return true; // нема поля — ок
    return v[key].isString() || v[key].isNull();
}

} // namespace

// ================== LIST ==================

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

// ================== CREATE ==================

void ShipTypesController::create(const HttpRequestPtr& req,
                                 std::function<void(const HttpResponsePtr&)>&& cb) {
    const auto j = req->getJsonObject();
    if (!j) {
        cb(jsonError("json body required", drogon::k400BadRequest));
        return;
    }

    if (!hasNonEmptyString(*j, "code") || !hasNonEmptyString(*j, "name")) {
        cb(jsonError("code and name are required", drogon::k400BadRequest));
        return;
    }

    if (!isStringOrNull(*j, "description")) {
        cb(jsonError("description must be string or null", drogon::k400BadRequest));
        return;
    }

    ShipType t;
    t.code = (*j)["code"].asString();
    t.name = (*j)["name"].asString();

    if ((*j).isMember("description")) {
        t.description = (*j)["description"].isNull()
                            ? ""
                            : (*j)["description"].asString();
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
        cb(jsonError("create failed", mapDbErrorToHttp(ex.what()), ex.what()));
    }
}

// ================== GET ONE ==================

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

// ================== UPDATE ==================

void ShipTypesController::updateOne(const HttpRequestPtr& req,
                                    std::function<void(const HttpResponsePtr&)>&& cb,
                                    std::int64_t id) {
    const auto j = req->getJsonObject();
    if (!j) {
        cb(jsonError("json body required", drogon::k400BadRequest));
        return;
    }

    if (!isStringOrNull(*j, "description")) {
        cb(jsonError("description must be string or null", drogon::k400BadRequest));
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
            t.description = (*j)["description"].isNull()
                                ? ""
                                : (*j)["description"].asString();
        }

        repo.update(t);

        cb(jsonOk("updated"));
    } catch (const std::exception& ex) {
        LOG_ERROR << "ShipTypesController::updateOne failed id=" << id
                  << ": " << ex.what();
        cb(jsonError("update failed", mapDbErrorToHttp(ex.what()), ex.what()));
    }
}

// ================== DELETE ==================

void ShipTypesController::deleteOne(const HttpRequestPtr&,
                                    std::function<void(const HttpResponsePtr&)>&& cb,
                                    std::int64_t id) {
    try {
        ShipTypesRepo repo;

        const auto cur = repo.byId(id);
        if (!cur) {
            cb(jsonError("not found", drogon::k404NotFound));
            return;
        }

        repo.remove(id);

        auto r = HttpResponse::newHttpResponse();
        r->setStatusCode(drogon::k204NoContent);
        cb(r);
    } catch (const std::exception& ex) {
        LOG_ERROR << "ShipTypesController::deleteOne failed id=" << id
                  << ": " << ex.what();
        cb(jsonError("delete failed", mapDbErrorToHttp(ex.what()), ex.what()));
    }
}
