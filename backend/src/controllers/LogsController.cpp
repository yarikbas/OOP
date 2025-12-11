#include "controllers/LogsController.h"
#include "db/Db.h"

#include <drogon/drogon.h>
#include <json/json.h>
#include <sqlite3.h>

using drogon::HttpRequestPtr;
using drogon::HttpResponse;
using drogon::HttpResponsePtr;

namespace {

// Simple token-based access control
const std::string EXPORT_TOKEN = "fleet-export-2025";

HttpResponsePtr jsonError(const std::string& msg, drogon::HttpStatusCode code) {
    Json::Value e;
    e["error"] = msg;
    auto r = HttpResponse::newHttpJsonResponse(e);
    r->setStatusCode(code);
    return r;
}

// Check authorization for export endpoints
bool checkExportAuth(const HttpRequestPtr& req) {
    // Check Authorization header or token query param
    std::string authHeader = req->getHeader("Authorization");
    if (!authHeader.empty() && authHeader.find("Bearer ") == 0) {
        std::string token = authHeader.substr(7); // Remove "Bearer "
        return token == EXPORT_TOKEN;
    }
    
    // Check query parameter as fallback
    std::string tokenParam = req->getParameter("token");
    if (!tokenParam.empty()) {
        return tokenParam == EXPORT_TOKEN;
    }
    
    return false;
}

// RAII for sqlite3_stmt
class Stmt {
public:
    Stmt(sqlite3* db, const char* sql) : db_(db), st_(nullptr) {
        if (sqlite3_prepare_v2(db_, sql, -1, &st_, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(db_));
        }
    }
    ~Stmt() { if (st_) sqlite3_finalize(st_); }
    sqlite3_stmt* get() const noexcept { return st_; }
private:
    sqlite3* db_;
    sqlite3_stmt* st_;
};

Json::Value rowToJson(sqlite3_stmt* st) {
    Json::Value obj(Json::objectValue);
    const int cols = sqlite3_column_count(st);
    for (int i = 0; i < cols; ++i) {
        const char* name = sqlite3_column_name(st, i);
        const int type = sqlite3_column_type(st, i);
        if (type == SQLITE_INTEGER) obj[name] = (Json::Int64)sqlite3_column_int64(st, i);
        else if (type == SQLITE_FLOAT) obj[name] = sqlite3_column_double(st, i);
        else {
            const unsigned char* txt = sqlite3_column_text(st, i);
            obj[name] = txt ? reinterpret_cast<const char*>(txt) : std::string();
        }
    }
    return obj;
}

// Export entire table generically (if exists)
Json::Value exportTable(sqlite3* db, const std::string& tableName) {
    Json::Value arr(Json::arrayValue);
    const std::string sql = "SELECT * FROM " + tableName;
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &st, nullptr) != SQLITE_OK) {
        return arr; // return empty if table missing or error
    }
    while (sqlite3_step(st) == SQLITE_ROW) {
        arr.append(rowToJson(st));
    }
    sqlite3_finalize(st);
    return arr;
}

} // namespace

void LogsController::list(const HttpRequestPtr& req,
                          std::function<void(const HttpResponsePtr&)>&& cb) {
    try {
        sqlite3* db = Db::instance().handle();

        // Collect filters from query params
        const auto eventType = req->getParameter("event_type");
        const auto level     = req->getParameter("level");
        const auto offsetStr = req->getParameter("offset");
        const auto entity    = req->getParameter("entity");
        const auto entityId  = req->getParameter("entity_id");
        const auto since     = req->getParameter("since");
        const auto until     = req->getParameter("until");
        int limit = 100;
        if (req->getParameter("limit").size()) {
            try { limit = std::stoi(req->getParameter("limit")); } catch(...) { }
        }

        std::string sql =
            "SELECT id, ts, level, event_type, entity, entity_id, user, message "
            "FROM logs WHERE 1=1";

        if (!level.empty())     sql += " AND level = ?";
        if (!eventType.empty()) sql += " AND event_type = ?";
        if (!entity.empty())    sql += " AND entity = ?";
        if (!entityId.empty())  sql += " AND entity_id = ?";
        if (!since.empty())     sql += " AND ts >= ?";
        if (!until.empty())     sql += " AND ts <= ?";
        sql += " ORDER BY ts DESC LIMIT ? OFFSET ?";

        Stmt st(db, sql.c_str());
        int idx = 1;
        if (!level.empty())     sqlite3_bind_text(st.get(), idx++, level.c_str(), -1, SQLITE_TRANSIENT);
        if (!eventType.empty()) sqlite3_bind_text(st.get(), idx++, eventType.c_str(), -1, SQLITE_TRANSIENT);
        if (!entity.empty())    sqlite3_bind_text(st.get(), idx++, entity.c_str(), -1, SQLITE_TRANSIENT);
        if (!entityId.empty())  sqlite3_bind_int64(st.get(), idx++, static_cast<long long>(std::stoll(entityId)));
        if (!since.empty())     sqlite3_bind_text(st.get(), idx++, since.c_str(), -1, SQLITE_TRANSIENT);
        if (!until.empty())     sqlite3_bind_text(st.get(), idx++, until.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(st.get(), idx++, limit);
        int offset = 0;
        if (!offsetStr.empty()) {
            try { offset = std::stoi(offsetStr); } catch(...) { offset = 0; }
        }
        sqlite3_bind_int(st.get(), idx++, offset);

        Json::Value arr(Json::arrayValue);
        while (sqlite3_step(st.get()) == SQLITE_ROW) {
            arr.append(rowToJson(st.get()));
        }

        // Log the query for audit
        try {
            std::string msg = "Queried logs: level=" + level + " event_type=" + eventType + " entity=" + entity + " entity_id=" + entityId + " since=" + since + " until=" + until + " limit=" + std::to_string(limit) + " offset=" + std::to_string(offset);
            Db::instance().insertLog("INFO", "logs.query", "logs", 0, "system", msg);
        } catch (...) {}

        cb(HttpResponse::newHttpJsonResponse(arr));
    }
    catch (const std::exception& e) {
        LOG_ERROR << "LogsController::list error: " << e.what();
        cb(jsonError("Internal Error", drogon::k500InternalServerError));
    }
}

void LogsController::exportData(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb) {
    try {
        // Check authorization
        if (!checkExportAuth(req)) {
            return cb(jsonError("Unauthorized: missing or invalid token", drogon::k401Unauthorized));
        }

        sqlite3* db = Db::instance().handle();

        Json::Value root(Json::objectValue);
        root["logs"] = exportTable(db, "logs");
        root["people"] = exportTable(db, "people");
        root["ships"]  = exportTable(db, "ships");
        root["companies"] = exportTable(db, "companies");
        root["ports"] = exportTable(db, "ports");

        // Log the export action
        try {
            Db::instance().insertLog("INFO", "export.data_full", "export", 0, "system", "Full data export requested");
        } catch (...) {}

        cb(HttpResponse::newHttpJsonResponse(root));
    }
    catch (const std::exception& e) {
        LOG_ERROR << "LogsController::exportData error: " << e.what();
        cb(jsonError("Internal Error", drogon::k500InternalServerError));
    }
}

void LogsController::exportCsv(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& cb) {
    try {
        // Check authorization
        if (!checkExportAuth(req)) {
            return cb(jsonError("Unauthorized: missing or invalid token", drogon::k401Unauthorized));
        }

        sqlite3* db = Db::instance().handle();

        // reuse filters from query params similar to list()
        const auto eventType = req->getParameter("event_type");
        const auto entity    = req->getParameter("entity");
        const auto entityId  = req->getParameter("entity_id");
        const auto since     = req->getParameter("since");
        const auto until     = req->getParameter("until");

        std::string sql =
            "SELECT id, ts, level, event_type, entity, entity_id, user, message "
            "FROM logs WHERE 1=1";

        if (!eventType.empty()) sql += " AND event_type = ?";
        if (!entity.empty())    sql += " AND entity = ?";
        if (!entityId.empty())  sql += " AND entity_id = ?";
        if (!since.empty())     sql += " AND ts >= ?";
        if (!until.empty())     sql += " AND ts <= ?";
        sql += " ORDER BY ts DESC";

        Stmt st(db, sql.c_str());
        int idx = 1;
        if (!eventType.empty()) sqlite3_bind_text(st.get(), idx++, eventType.c_str(), -1, SQLITE_TRANSIENT);
        if (!entity.empty())    sqlite3_bind_text(st.get(), idx++, entity.c_str(), -1, SQLITE_TRANSIENT);
        if (!entityId.empty())  sqlite3_bind_int64(st.get(), idx++, static_cast<long long>(std::stoll(entityId)));
        if (!since.empty())     sqlite3_bind_text(st.get(), idx++, since.c_str(), -1, SQLITE_TRANSIENT);
        if (!until.empty())     sqlite3_bind_text(st.get(), idx++, until.c_str(), -1, SQLITE_TRANSIENT);

        // build CSV
        std::string csv = "id,ts,level,event_type,entity,entity_id,user,message\n";
        while (sqlite3_step(st.get()) == SQLITE_ROW) {
            // get columns
            long long id = sqlite3_column_int64(st.get(), 0);
            const unsigned char* ts = sqlite3_column_text(st.get(), 1);
            const unsigned char* level = sqlite3_column_text(st.get(), 2);
            const unsigned char* ev = sqlite3_column_text(st.get(), 3);
            const unsigned char* en = sqlite3_column_text(st.get(), 4);
            long long eid = sqlite3_column_int64(st.get(), 5);
            const unsigned char* user = sqlite3_column_text(st.get(), 6);
            const unsigned char* msg = sqlite3_column_text(st.get(), 7);

            auto esc = [](const unsigned char* s){ if(!s) return std::string(); std::string t = reinterpret_cast<const char*>(s); for(auto &c:t){ if(c=='\n') c=' '; if(c=='\r') c=' '; } return t; };

            csv += std::to_string(id) + "," + (ts?esc(ts):std::string()) + "," + (level?esc(level):std::string()) + "," + (ev?esc(ev):std::string()) + "," + (en?esc(en):std::string()) + "," + std::to_string(eid) + "," + (user?esc(user):std::string()) + ",\"" + (msg?esc(msg):std::string()) + "\"\n";
        }

        // log the export action
        try {
            std::string msg = "Exported logs CSV (auth: token)";
            Db::instance().insertLog("INFO", "logs.export_csv", "logs", 0, "system", msg);
        } catch(...) {}

        auto r = HttpResponse::newHttpResponse();
        r->setStatusCode(drogon::k200OK);
        r->setBody(csv);
        r->setContentTypeCode(drogon::ContentType::CT_TEXT_CSV);
        cb(r);
    }
    catch (const std::exception& e) {
        LOG_ERROR << "LogsController::exportCsv error: " << e.what();
        cb(jsonError("Internal Error", drogon::k500InternalServerError));
    }
}
