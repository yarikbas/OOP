#include "controllers/OopDemoController.h"
#include "domain/Engineer.h"
#include "domain/Researcher.h"
#include "domain/Soldier.h"
#include "domain/ShipDomain.h"
#include <json/json.h>
#include <memory>
#include <vector>

using namespace drogon;

void OopDemoController::people(const HttpRequestPtr&, std::function<void(const HttpResponsePtr&)>&& cb) {
    std::vector<std::unique_ptr<Person>> team;
    team.emplace_back(std::make_unique<Engineer>("Alice", "Electrical"));
    team.emplace_back(std::make_unique<Researcher>("Bob", "Oceanography"));
    team.emplace_back(std::make_unique<Soldier>("Eve", "Lieutenant"));

    Json::Value arr(Json::arrayValue);
    for (auto& p : team) {
        Json::Value j;
        j["name"] = p->name();
        j["role"] = p->role();
        j["duty"] = p->duty();
        if (auto e = dynamic_cast<Engineer*>(p.get()))   j["specialty"] = e->specialty();
        if (auto r = dynamic_cast<Researcher*>(p.get())) j["field"]     = r->field();
        if (auto s = dynamic_cast<Soldier*>(p.get()))    j["rank"]      = s->rank();
        arr.append(j);
    }
    cb(HttpResponse::newHttpJsonResponse(arr));
}

void OopDemoController::ships(const HttpRequestPtr&, std::function<void(const HttpResponsePtr&)>&& cb) {
    std::vector<std::unique_ptr<ShipBase>> fleet;
    fleet.emplace_back(std::make_unique<CargoShip>("Mriya Sea", 12000.0, 8000.0));
    fleet.emplace_back(std::make_unique<MilitaryShip>("Defender", 15000.0, 12));
    fleet.emplace_back(std::make_unique<ResearchShip>("Explorer", 9000.0, 3));

    Json::Value arr(Json::arrayValue);
    for (auto& s : fleet) {
        Json::Value j;
        j["name"]     = s->name();
        j["tonnage"]  = s->tonnage();
        j["category"] = s->category();
        j["mission"]  = s->mission();
        if (auto c = dynamic_cast<CargoShip*>(s.get()))     j["capacity_tons"] = c->capacityTons();
        if (auto m = dynamic_cast<MilitaryShip*>(s.get()))  j["weapons"]       = m->weapons();
        if (auto r = dynamic_cast<ResearchShip*>(s.get()))  j["labs"]          = r->labs();
        arr.append(j);
    }
    cb(HttpResponse::newHttpJsonResponse(arr));
}
