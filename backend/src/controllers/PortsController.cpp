// src/controllers/PortsController.cpp
#include "controllers/PortsController.h"
#include "repos/PortsRepo.h"

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
        e["details"] = details; // корисно під час налагодження
    }
    auto r = HttpResponse::newHttpJsonResponse(e);
    r->setStatusCode(code);
    return r;
}

Json::Value portToJson(const Port& p) {
    Json::Value j;
    j["id"]     = Json::Int64(p.id);
    j["name"]   = p.name;
    j["region"] = p.region;
    j["lat"]    = p.lat;
    j["lon"]    = p.lon;
    return j;
}

bool hasNonEmptyString(const Json::Value& v, const char* key) {
    return v.isMember(key) && v[key].isString() && !v[key].asString().empty();
}

bool hasNumber(const Json::Value& v, const char* key) {
    return v.isMember(key) &&
           (v[key].isDouble() || v[key].isInt() || v[key].isUInt() || v[key].isIntegral());
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

} // namespace

// ================== LIST ==================

void PortsController::list(const HttpRequestPtr&,
                           std::function<void(const HttpResponsePtr&)>&& cb) {
    try {
        PortsRepo repo;
        const auto ports = repo.all();

        Json::Value arr(Json::arrayValue);
        for (const auto& p : ports) {
            arr.append(portToJson(p));
        }

        cb(HttpResponse::newHttpJsonResponse(arr));
    } catch (const std::exception& e) {
        LOG_ERROR << "PortsController::list failed: " << e.what();
        cb(jsonError("list failed", drogon::k500InternalServerError, e.what()));
    }
}

// ================== CREATE ==================

void PortsController::create(const HttpRequestPtr& req,
                             std::function<void(const HttpResponsePtr&)>&& cb) {
    const auto json = req->getJsonObject();
    if (!json) {
        cb(jsonError("json body required", drogon::k400BadRequest));
        return;
    }

    const auto& body = *json;

    if (!hasNonEmptyString(body, "name")) {
        cb(jsonError("name is required", drogon::k400BadRequest));
        return;
    }
    if (!hasNonEmptyString(body, "region")) {
        cb(jsonError("region is required", drogon::k400BadRequest));
        return;
    }
    if (!hasNumber(body, "lat") || !hasNumber(body, "lon")) {
        cb(jsonError("lat and lon are required", drogon::k400BadRequest));
        return;
    }

    Port p;
    p.name   = body["name"].asString();
    p.region = body["region"].asString();
    p.lat    = body["lat"].asDouble();
    p.lon    = body["lon"].asDouble();

    try {
        PortsRepo repo;
        const auto created = repo.create(p);

        auto resp = HttpResponse::newHttpJsonResponse(portToJson(created));
        resp->setStatusCode(drogon::k201Created);
        cb(resp);
    } catch (const std::exception& e) {
        LOG_ERROR << "PortsController::create failed name='" << p.name
                  << "': " << e.what();
        cb(jsonError("create failed", mapDbErrorToHttp(e.what()), e.what()));
    }
}

// ================== GET ONE ==================

void PortsController::getOne(const HttpRequestPtr&,
                             std::function<void(const HttpResponsePtr&)>&& cb,
                             int64_t id) {
    try {
        PortsRepo repo;
        const auto portOpt = repo.getById(id);

        if (!portOpt) {
            cb(jsonError("not found", drogon::k404NotFound));
            return;
        }

        cb(HttpResponse::newHttpJsonResponse(portToJson(*portOpt)));
    } catch (const std::exception& e) {
        LOG_ERROR << "PortsController::getOne failed id=" << id
                  << ": " << e.what();
        cb(jsonError("get failed", drogon::k500InternalServerError, e.what()));
    }
}

// ================== UPDATE ==================

void PortsController::update(const HttpRequestPtr& req,
                             std::function<void(const HttpResponsePtr&)>&& cb,
                             int64_t id) {
    const auto json = req->getJsonObject();
    if (!json) {
        cb(jsonError("json body required", drogon::k400BadRequest));
        return;
    }

    const auto& body = *json;

    try {
        PortsRepo repo;
        const auto portOpt = repo.getById(id);

        if (!portOpt) {
            cb(jsonError("not found", drogon::k404NotFound));
            return;
        }

        Port p = *portOpt;

        if (body.isMember("name")) {
            if (!body["name"].isString() || body["name"].asString().empty()) {
                cb(jsonError("name must be non-empty string", drogon::k400BadRequest));
                return;
            }
            p.name = body["name"].asString();
        }

        if (body.isMember("region")) {
            if (!body["region"].isString() || body["region"].asString().empty()) {
                cb(jsonError("region must be non-empty string", drogon::k400BadRequest));
                return;
            }
            p.region = body["region"].asString();
        }

        if (body.isMember("lat")) {
            if (!hasNumber(body, "lat")) {
                cb(jsonError("lat must be number", drogon::k400BadRequest));
                return;
            }
            p.lat = body["lat"].asDouble();
        }

        if (body.isMember("lon")) {
            if (!hasNumber(body, "lon")) {
                cb(jsonError("lon must be number", drogon::k400BadRequest));
                return;
            }
            p.lon = body["lon"].asDouble();
        }

        // ВАЖЛИВО:
        // з новим PortsRepo::update:
        // - кидає exception при помилці
        // - повертає true навіть якщо значення ті самі
        repo.update(p);

        cb(HttpResponse::newHttpJsonResponse(portToJson(p)));
    } catch (const std::exception& e) {
        LOG_ERROR << "PortsController::update failed id=" << id
                  << ": " << e.what();
        cb(jsonError("update failed", mapDbErrorToHttp(e.what()), e.what()));
    }
}

// ================== REMOVE ==================

void PortsController::remove(const HttpRequestPtr&,
                             std::function<void(const HttpResponsePtr&)>&& cb,
                             int64_t id) {
    try {
        PortsRepo repo;

        // Явно перевіряємо існування
        const auto portOpt = repo.getById(id);
        if (!portOpt) {
            cb(jsonError("not found", drogon::k404NotFound));
            return;
        }

        // З новим PortsRepo::remove:
        // - false = реально не знайдено (малоймовірно після перевірки)
        // - FK/інші проблеми -> exception
        const bool ok = repo.remove(id);
        if (!ok) {
            cb(jsonError("not found", drogon::k404NotFound));
            return;
        }

        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k204NoContent);
        cb(resp);
    } catch (const std::exception& e) {
        LOG_ERROR << "PortsController::remove failed id=" << id
                  << ": " << e.what();
        cb(jsonError("remove failed", mapDbErrorToHttp(e.what()), e.what()));
    }
}
