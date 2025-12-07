// src/controllers/CompaniesController.cpp
#include "controllers/CompaniesController.h"
#include "repos/CompaniesRepo.h"

#include <drogon/drogon.h>
#include <json/json.h>

#include <algorithm>
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
        e["details"] = details; // корисно на етапі дебагу
    }
    auto r = HttpResponse::newHttpJsonResponse(e);
    r->setStatusCode(code);
    return r;
}

HttpResponsePtr jsonOk(const std::string& status) {
    Json::Value out;
    out["status"] = status;
    return HttpResponse::newHttpJsonResponse(out);
}

// ---------------- DTO -> JSON ----------------

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

// ---------------- Error mapping ----------------

HttpStatusCode statusFromSqliteMessage(const std::string& msg) {
    if (msg.find("UNIQUE") != std::string::npos || msg.find("unique") != std::string::npos)
        return drogon::k409Conflict;

    if (msg.find("FOREIGN KEY") != std::string::npos || msg.find("foreign key") != std::string::npos)
        return drogon::k409Conflict;

    if (msg.find("NOT NULL") != std::string::npos || msg.find("not null") != std::string::npos)
        return drogon::k400BadRequest;

    return drogon::k500InternalServerError;
}

// ---------------- Validation helpers ----------------

bool hasNonEmptyName(const Json::Value& j) {
    return j.isMember("name") && j["name"].isString() && !j["name"].asString().empty();
}

bool isIntegral(const Json::Value& v) {
    return v.isInt() || v.isUInt() || v.isInt64() || v.isUInt64();
}

// API-синонімія: is_hq == is_main
bool readIsMain(const Json::Value& j, bool& out) {
    if (j.isMember("is_main")) {
        if (!j["is_main"].isBool()) return false;
        out = j["is_main"].asBool();
        return true;
    }
    if (j.isMember("is_hq")) {
        if (!j["is_hq"].isBool()) return false;
        out = j["is_hq"].asBool();
        return true;
    }
    out = false;
    return true;
}

} // namespace

// ================== LIST ==================

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

// ================== CREATE ==================

void CompaniesController::create(const HttpRequestPtr& req,
                                 std::function<void(const HttpResponsePtr&)>&& cb) {
    const auto j = req->getJsonObject();
    if (!j || !hasNonEmptyName(*j)) {
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
        cb(jsonError("create failed", statusFromSqliteMessage(e.what()), e.what()));
    } catch (...) {
        LOG_ERROR << "CompaniesController::create failed with unknown exception for name='"
                  << name << "'";
        cb(jsonError("create failed", drogon::k500InternalServerError));
    }
}

// ================== GET ONE ==================

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

// ================== UPDATE ==================

void CompaniesController::update(const HttpRequestPtr& req,
                                 std::function<void(const HttpResponsePtr&)>&& cb,
                                 std::int64_t id) {
    const auto j = req->getJsonObject();
    if (!j || !hasNonEmptyName(*j)) {
        cb(jsonError("name required", drogon::k400BadRequest));
        return;
    }

    const auto name = (*j)["name"].asString();

    try {
        CompaniesRepo repo;

        // 1) 404 якщо компанії нема
        const auto cur = repo.byId(id);
        if (!cur) {
            cb(jsonError("not found", drogon::k404NotFound));
            return;
        }

        // 2) Точний "unchanged" без ризику сховати помилку
        if (cur->name == name) {
            cb(jsonOk("unchanged"));
            return;
        }

        // 3) Додаткова перевірка на зайняте ім'я (без очікування на rc)
        const auto allCompanies = repo.all();
        const bool nameTaken =
            std::any_of(allCompanies.begin(), allCompanies.end(),
                        [&](const Company& c) {
                            return c.id != id && c.name == name;
                        });

        if (nameTaken) {
            cb(jsonError("name already exists", drogon::k409Conflict));
            return;
        }

        // 4) Оновлення
        const bool ok = repo.update(id, name);
        if (!ok) {
            // З урахуванням реалізації репо це означає збій/аномалію,
            // бо ми вже відсікли "unchanged" і "nameTaken".
            cb(jsonError("update failed", drogon::k500InternalServerError));
            return;
        }

        cb(jsonOk("updated"));
    } catch (const std::exception& e) {
        LOG_ERROR << "CompaniesController::update failed id=" << id << ": " << e.what();
        cb(jsonError("update failed", statusFromSqliteMessage(e.what()), e.what()));
    }
}

// ================== REMOVE ==================

void CompaniesController::remove(const HttpRequestPtr&,
                                 std::function<void(const HttpResponsePtr&)>&& cb,
                                 std::int64_t id) {
    try {
        CompaniesRepo repo;

        // 1) 404 якщо компанії нема
        const auto cur = repo.byId(id);
        if (!cur) {
            cb(jsonError("not found", drogon::k404NotFound));
            return;
        }

        // 2) Видаляємо
        const bool ok = repo.remove(id);
        if (!ok) {
            // У твоєму репо false може означати exception.
            cb(jsonError("remove failed", drogon::k500InternalServerError));
            return;
        }

        auto r = HttpResponse::newHttpResponse();
        r->setStatusCode(drogon::k204NoContent);
        cb(r);
    } catch (const std::exception& e) {
        LOG_ERROR << "CompaniesController::remove failed id=" << id << ": " << e.what();
        cb(jsonError("remove failed", statusFromSqliteMessage(e.what()), e.what()));
    }
}

// ================== LIST PORTS ==================

void CompaniesController::listPorts(const HttpRequestPtr&,
                                    std::function<void(const HttpResponsePtr&)>&& cb,
                                    std::int64_t id) {
    try {
        CompaniesRepo repo;

        const auto c = repo.byId(id);
        if (!c) {
            cb(jsonError("not found", drogon::k404NotFound));
            return;
        }

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

// ================== ADD PORT ==================

void CompaniesController::addPort(const HttpRequestPtr& req,
                                  std::function<void(const HttpResponsePtr&)>&& cb,
                                  std::int64_t id) {
    const auto j = req->getJsonObject();
    if (!j || !(*j).isMember("port_id") || !isIntegral((*j)["port_id"])) {
        cb(jsonError("port_id required", drogon::k400BadRequest));
        return;
    }

    const auto portId = (*j)["port_id"].asInt64();
    if (portId <= 0) {
        cb(jsonError("port_id must be positive", drogon::k400BadRequest));
        return;
    }

    bool isMain = false;
    if (!readIsMain(*j, isMain)) {
        cb(jsonError("is_hq/is_main must be bool", drogon::k400BadRequest));
        return;
    }

    try {
        CompaniesRepo repo;

        const auto c = repo.byId(id);
        if (!c) {
            cb(jsonError("not found", drogon::k404NotFound));
            return;
        }

        // ВАЖЛИВО:
        // repo.addPort робить UPSERT і вміє оновити is_main,
        // тому "already exists" НЕ має бути 409 на рівні контролера.
        const bool ok = repo.addPort(id, portId, isMain);
        if (!ok) {
            cb(jsonError("invalid company/port or constraint", drogon::k400BadRequest));
            return;
        }

        cb(jsonOk("added"));
    } catch (const std::exception& e) {
        LOG_ERROR << "CompaniesController::addPort failed companyId=" << id
                  << " portId=" << portId << ": " << e.what();
        cb(jsonError("add port failed", statusFromSqliteMessage(e.what()), e.what()));
    }
}

// ================== DELETE PORT ==================

void CompaniesController::delPort(const HttpRequestPtr&,
                                  std::function<void(const HttpResponsePtr&)>&& cb,
                                  std::int64_t id,
                                  std::int64_t portId) {
    try {
        CompaniesRepo repo;

        const auto c = repo.byId(id);
        if (!c) {
            cb(jsonError("not found", drogon::k404NotFound));
            return;
        }

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
        cb(jsonError("delete port failed", statusFromSqliteMessage(e.what()), e.what()));
    }
}

// ================== LIST SHIPS ==================

void CompaniesController::listShips(const HttpRequestPtr&,
                                    std::function<void(const HttpResponsePtr&)>&& cb,
                                    std::int64_t id) {
    try {
        CompaniesRepo repo;

        const auto c = repo.byId(id);
        if (!c) {
            cb(jsonError("not found", drogon::k404NotFound));
            return;
        }

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
