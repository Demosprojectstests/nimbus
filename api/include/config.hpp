#pragma once
#include <cstdlib>
#include <stdexcept>
#include <string>

inline std::string env_or(const char* key, const std::string& fallback) {
    const char* v = std::getenv(key);
    return (v && *v) ? std::string(v) : fallback;
}

inline std::string env_or_throw(const char* key) {
    const char* v = std::getenv(key);
    if (!v || !*v) throw std::runtime_error(std::string("missing env: ") + key);
    return v;
}

struct Config {
    std::string jwt_secret = env_or("JWT_SECRET", "dev-only-change-me-now");
    std::string db_path    = env_or("DB_PATH", "./nimbus.db");
    std::string bind_host  = env_or("BIND_HOST", "127.0.0.1");
    int port               = std::stoi(env_or("PORT", "8080"));
    int jwt_ttl_minutes    = 15;
};
