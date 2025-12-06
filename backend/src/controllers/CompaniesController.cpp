#include "controllers/CompaniesController.h"
#include "repos/CompaniesRepo.h"

#include <drogon/drogon.h>
#include <json/json.h>

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
        e["details"] = details; // корисно на етапі дебагу
    }
    auto r = HttpResponse::newHttpJsonResponse(e);
    r->setStatusCode(code);
    return r;
}

Json::Value companyToJson(const Company& c) {
    Json::Value j;
    j["id"]   = Json::Int64(c.id);
    j["name"] = c.name;
    return j;
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

Json::Value shipToJson(const Ship& s) {
    Json::Value j;
    j["id"]         = Json::Int64(s.id);
    j["name"]       = s.name;
    j["type"]       = s.type;
    j["country"]    = s.country;
    j["port_id"]    = Json::Int64(s.port_id);
    j["status"]     = s.status;
    j["company_id"] = Json::Int64(s.company_id);
    return j;
}

HttpStatusCode statusFromSqliteMessage(const std::string& msg) {
    // мінімальна евристика — достатньо для MVP
    if (msg.find("UNIQUE") != std::string::npos)   return drogon::k409Conflict;
    if (msg.find("NOT NULL") != std::string::npos) return drogon::k400BadRequest;
    if (msg.find("FOREIGN KEY") != std::string::npos) return drogon::k400BadRequest;
    return drogon::k500InternalServerError;
}

} // namespace

void CompaniesController::list(const HttpRequestPtr&,
                               std::function<void(const HttpResponsePtr&)>&& cb) {
    try {
        CompaniesRepo repo;
        const auto vec = repo.all();

        Json::Value arr(Json::arrayValue);
        for (const auto& c : vec) {
            arr.append(companyToJson(c));
        }
        cb(HttpResponse::newHttpJsonResponse(arr));
    } catch (const std::exception& e) {
        LOG_ERROR << "CompaniesController::list failed: " << e.what();
        cb(jsonError("list failed", drogon::k500InternalServerError, e.what()));
    }
}

void CompaniesController::create(const HttpRequestPtr& req,
                                 std::function<void(const HttpResponsePtr&)>&& cb) {
    const auto j = req->getJsonObject();
    if (!j || !(*j)["name"].isString() || (*j)["name"].asString().empty()) {
        cb(jsonError("name required", drogon::k400BadRequest));
        return;
    }

    const auto name = (*j)["name"].asString();

    try {
        CompaniesRepo repo;
        const auto c = repo.create(name);

        auto resp = HttpResponse::newHttpJsonResponse(companyToJson(c));
        resp->setStatusCode(drogon::k201Created);
        cb(resp);
    } catch (const std::exception& e) {
        LOG_ERROR << "CompaniesController::create failed for name='"
                  << name << "': " << e.what();

        const std::string msg = e.what();
        cb(jsonError("create failed", statusFromSqliteMessage(msg), msg));
    } catch (...) {
        LOG_ERROR << "CompaniesController::create failed with unknown exception for name='"
                  << name << "'";
        cb(jsonError("create failed", drogon::k500InternalServerError));
    }
}

void CompaniesController::getOne(const HttpRequestPtr&,
                                 std::function<void(const HttpResponsePtr&)>&& cb,
                                 std::int64_t id) {
    try {
        CompaniesRepo repo;
        const auto c = repo.byId(id);
        if (!c) {
            cb(jsonError("not found", drogon::k404NotFound));
            return;
        }
        cb(HttpResponse::newHttpJsonResponse(companyToJson(*c)));
    } catch (const std::exception& e) {
        LOG_ERROR << "CompaniesController::getOne failed id=" << id << ": " << e.what();
        cb(jsonError("get failed", drogon::k500InternalServerError, e.what()));
    }
}

void CompaniesController::update(const HttpRequestPtr& req,
                                 std::function<void(const HttpResponsePtr&)>&& cb,
                                 std::int64_t id) {
    const auto j = req->getJsonObject();
    if (!j || !(*j)["name"].isString() || (*j)["name"].asString().empty()) {
        cb(jsonError("name required", drogon::k400BadRequest));
        return;
    }

    const auto name = (*j)["name"].asString();

    try {
        CompaniesRepo repo;
        const bool ok = repo.update(id, name);
        if (!ok) {
            // у тебе repo повертає false і коли не знайдено, і коли нічого не змінилось
            cb(jsonError("not found or unchanged", drogon::k404NotFound));
            return;
        }

        Json::Value out;
        out["status"] = "updated";
        cb(HttpResponse::newHttpJsonResponse(out));
    } catch (const std::exception& e) {
        LOG_ERROR << "CompaniesController::update failed id=" << id << ": " << e.what();
        cb(jsonError("update failed", statusFromSqliteMessage(e.what()), e.what()));
    }
}

void CompaniesController::remove(const HttpRequestPtr&,
                                 std::function<void(const HttpResponsePtr&)>&& cb,
                                 std::int64_t id) {
    try {
        CompaniesRepo repo;
        const bool ok = repo.remove(id);
        if (!ok) {
            cb(jsonError("not found", drogon::k404NotFound));
            return;
        }
        auto r = HttpResponse::newHttpResponse();
        r->setStatusCode(drogon::k204NoContent);
        cb(r);
    } catch (const std::exception& e) {
        LOG_ERROR << "CompaniesController::remove failed id=" << id << ": " << e.what();
        cb(jsonError("remove failed", drogon::k500InternalServerError, e.what()));
    }
}

void CompaniesController::listPorts(const HttpRequestPtr&,
                                    std::function<void(const HttpResponsePtr&)>&& cb,
                                    std::int64_t id) {
    try {
        CompaniesRepo repo;
        const auto vec = repo.ports(id);

        Json::Value arr(Json::arrayValue);
        for (const auto& p : vec) {
            arr.append(portToJson(p));
        }
        cb(HttpResponse::newHttpJsonResponse(arr));
    } catch (const std::exception& e) {
        LOG_ERROR << "CompaniesController::listPorts failed id=" << id << ": " << e.what();
        cb(jsonError("list ports failed", drogon::k500InternalServerError, e.what()));
    }
}

void CompaniesController::addPort(const HttpRequestPtr& req,
                                  std::function<void(const HttpResponsePtr&)>&& cb,
                                  std::int64_t id) {
    const auto j = req->getJsonObject();
    if (!j || !(*j)["port_id"].isIntegral()) {
        cb(jsonError("port_id required", drogon::k400BadRequest));
        return;
    }

    const auto portId = (*j)["port_id"].asInt64();
    const bool isHq = (*j).isMember("is_hq") ? (*j)["is_hq"].asBool() : false;

    try {
        CompaniesRepo repo;
        const bool ok = repo.addPort(id, portId, isHq);
        if (!ok) {
            cb(jsonError("already exists or invalid", drogon::k400BadRequest));
            return;
        }

        Json::Value out;
        out["status"] = "added";
        cb(HttpResponse::newHttpJsonResponse(out));
    } catch (const std::exception& e) {
        LOG_ERROR << "CompaniesController::addPort failed companyId=" << id
                  << " portId=" << portId << ": " << e.what();
        cb(jsonError("add port failed", statusFromSqliteMessage(e.what()), e.what()));
    }
}

void CompaniesController::delPort(const HttpRequestPtr&,
                                  std::function<void(const HttpResponsePtr&)>&& cb,
                                  std::int64_t id,
                                  std::int64_t portId) {
    try {
        CompaniesRepo repo;
        const bool ok = repo.removePort(id, portId);
        if (!ok) {
            cb(jsonError("not found", drogon::k404NotFound));
            return;
        }
        auto r = HttpResponse::newHttpResponse();
        r->setStatusCode(drogon::k204NoContent);
        cb(r);
    } catch (const std::exception& e) {
        LOG_ERROR << "CompaniesController::delPort failed companyId=" << id
                  << " portId=" << portId << ": " << e.what();
        cb(jsonError("delete port failed", drogon::k500InternalServerError, e.what()));
    }
}

void CompaniesController::listShips(const HttpRequestPtr&,
                                    std::function<void(const HttpResponsePtr&)>&& cb,
                                    std::int64_t id) {
    try {
        CompaniesRepo repo;
        const auto vec = repo.ships(id);

        Json::Value arr(Json::arrayValue);
        for (const auto& s : vec) {
            arr.append(shipToJson(s));
        }
        cb(HttpResponse::newHttpJsonResponse(arr));
    } catch (const std::exception& e) {
        LOG_ERROR << "CompaniesController::listShips failed id=" << id << ": " << e.what();
        cb(jsonError("list ships failed", drogon::k500InternalServerError, e.what()));
    }
}
