#include "crow.h"
#include "config.hpp"
#include "db.hpp"
#include "auth.hpp"
#include "validation.hpp"

static crow::response json_err(int code, const std::string& msg) {
    crow::json::wvalue j;
    j["error"] = msg;
    return {code, j};
}

static std::optional<TokenClaims> require_auth(const crow::request& req, const Config& cfg) {
    auto tok = bearer_from(req.get_header_value("Authorization"));
    if (!tok) return std::nullopt;
    return verify_jwt(cfg, *tok);
}

int main() {
    Config cfg;
    Database db(cfg.db_path);
    db.migrate();

    crow::SimpleApp app;

    CROW_ROUTE(app, "/health").methods("GET"_method)([] {
        crow::json::wvalue j;
        j["status"] = "ok";
        j["service"] = "nimbus-api";
        return j;
    });

    CROW_ROUTE(app, "/api/v1/auth/register").methods("POST"_method)
    ([&](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return json_err(400, "invalid json");
        std::string username = body["username"].s();
        std::string password = body["password"].s();
        if (!valid_username(username) || !valid_password(password))
            return json_err(400, "invalid username or password policy");
        auto [hash, salt] = hash_password(password);
        if (!db.create_user(username, hash, salt, "user"))
            return json_err(409, "username taken");
        return crow::response{201};
    });

    CROW_ROUTE(app, "/api/v1/auth/login").methods("POST"_method)
    ([&](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return json_err(400, "invalid json");
        auto user = db.find_user(body["username"].s());
        if (!user || !verify_password(body["password"].s(), user->salt, user->password_hash))
            return json_err(401, "invalid credentials");
        TokenClaims c{user->id, user->username, user->role};
        crow::json::wvalue j;
        j["token"] = issue_jwt(cfg, c);
        j["role"] = user->role;
        return crow::response{j};
    });

    CROW_ROUTE(app, "/api/v1/me").methods("GET"_method)
    ([&](const crow::request& req) {
        auto claims = require_auth(req, cfg);
        if (!claims) return json_err(401, "unauthorized");
        crow::json::wvalue j;
        j["id"] = claims->user_id;
        j["username"] = claims->username;
        j["role"] = claims->role;
        return crow::response{j};
    });

    CROW_ROUTE(app, "/api/v1/records").methods("GET"_method)
    ([&](const crow::request& req) {
        auto c = require_auth(req, cfg);
        if (!c) return json_err(401, "unauthorized");
        auto rows = db.list_records(c->user_id, c->role == "admin");
        crow::json::wvalue arr = crow::json::wvalue::list();
        for (size_t i = 0; i < rows.size(); ++i) {
            arr[i]["id"] = rows[i].id;
            arr[i]["owner_id"] = rows[i].owner_id;
            arr[i]["title"] = rows[i].title;
            arr[i]["body"] = rows[i].body;
        }
        return crow::response{arr};
    });

    CROW_ROUTE(app, "/api/v1/records").methods("POST"_method)
    ([&](const crow::request& req) {
        auto c = require_auth(req, cfg);
        if (!c) return json_err(401, "unauthorized");
        auto body = crow::json::load(req.body);
        if (!body) return json_err(400, "invalid json");
        std::string title = body["title"].s();
        std::string text = body["body"].s();
        if (!valid_title(title) || text.size() > 4000)
            return json_err(400, "invalid payload");
        auto rec = db.create_record(c->user_id, title, text);
        if (!rec) return json_err(500, "create failed");
        crow::json::wvalue j;
        j["id"] = rec->id;
        return crow::response{201, j};
    });

    CROW_ROUTE(app, "/api/v1/records/<int>").methods("DELETE"_method)
    ([&](const crow::request& req, int id) {
        auto c = require_auth(req, cfg);
        if (!c) return json_err(401, "unauthorized");
        bool ok = db.delete_record(id, c->user_id, c->role == "admin");
        return ok ? crow::response{204} : json_err(404, "not found");
    });

    app.bindaddr(cfg.bind_host).port(cfg.port).multithreaded().run();
}
