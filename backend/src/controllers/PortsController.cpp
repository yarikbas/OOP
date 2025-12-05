#include "controllers/PortsController.h"
#include "repos/PortsRepo.h"
#include <json/json.h>
using namespace drogon;

void PortsController::list(const HttpRequestPtr&,
                           std::function<void(const HttpResponsePtr&)>&& cb)
{
    PortsRepo repo;
    auto ports = repo.all();
    Json::Value arr(Json::arrayValue);
    for (const auto& p : ports) {
        Json::Value j;
        j["id"]     = Json::Int64(p.id);
        j["name"]   = p.name;
        j["region"] = p.region;
        j["lat"]    = p.lat;
        j["lon"]    = p.lon;
        arr.append(j);
    }
    cb(HttpResponse::newHttpJsonResponse(arr));
}
