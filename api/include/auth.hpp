#pragma once
#include "config.hpp"
#include <optional>
#include <string>
#include <utility>

struct TokenClaims {
    int user_id{};
    std::string username;
    std::string role;
};

std::pair<std::string, std::string> hash_password(const std::string& password);
bool verify_password(const std::string& password, const std::string& salt_hex,
                     const std::string& hash_hex);
std::string issue_jwt(const Config& cfg, const TokenClaims& claims);
std::optional<TokenClaims> verify_jwt(const Config& cfg, const std::string& token);
std::optional<std::string> bearer_from(const std::string& header);
