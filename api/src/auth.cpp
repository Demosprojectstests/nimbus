#include "auth.hpp"
#include <jwt-cpp/jwt.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <vector>

static std::string to_hex(const unsigned char* p, size_t n) {
    std::ostringstream os;
    os << std::hex << std::setfill('0');
    for (size_t i = 0; i < n; ++i) os << std::setw(2) << static_cast<int>(p[i]);
    return os.str();
}

static std::vector<unsigned char> from_hex(const std::string& hex) {
    std::vector<unsigned char> out(hex.size() / 2);
    for (size_t i = 0; i < out.size(); ++i)
        out[i] = static_cast<unsigned char>(std::stoi(hex.substr(i * 2, 2), nullptr, 16));
    return out;
}

std::pair<std::string, std::string> hash_password(const std::string& password) {
    unsigned char salt[16];
    RAND_bytes(salt, sizeof(salt));
    unsigned char hash[32];
    PKCS5_PBKDF2_HMAC(password.c_str(), static_cast<int>(password.size()),
                      salt, sizeof(salt), 210000, EVP_sha256(), sizeof(hash), hash);
    return {to_hex(hash, sizeof(hash)), to_hex(salt, sizeof(salt))};
}

bool verify_password(const std::string& password, const std::string& salt_hex,
                     const std::string& hash_hex) {
    auto salt = from_hex(salt_hex);
    unsigned char hash[32];
    PKCS5_PBKDF2_HMAC(password.c_str(), static_cast<int>(password.size()),
                      salt.data(), static_cast<int>(salt.size()),
                      210000, EVP_sha256(), sizeof(hash), hash);
    return to_hex(hash, sizeof(hash)) == hash_hex;
                     }

                     std::string issue_jwt(const Config& cfg, const TokenClaims& c) {
                         return jwt::create()
                         .set_issuer("nimbus")
                         .set_type("JWT")
                         .set_issued_at(std::chrono::system_clock::now())
                         .set_expires_at(std::chrono::system_clock::now() +
                         std::chrono::minutes{cfg.jwt_ttl_minutes})
                         .set_payload_claim("uid", jwt::claim(std::to_string(c.user_id)))
                         .set_payload_claim("sub", jwt::claim(c.username))
                         .set_payload_claim("role", jwt::claim(c.role))
                         .sign(jwt::algorithm::hs256{cfg.jwt_secret});
                     }

                     std::optional<TokenClaims> verify_jwt(const Config& cfg, const std::string& token) {
                         try {
                             auto decoded = jwt::decode(token);
                             jwt::verify()
                             .allow_algorithm(jwt::algorithm::hs256{cfg.jwt_secret})
                             .with_issuer("nimbus")
                             .verify(decoded);
                             TokenClaims c;
                             c.user_id = std::stoi(decoded.get_payload_claim("uid").as_string());
                             c.username = decoded.get_payload_claim("sub").as_string();
                             c.role = decoded.get_payload_claim("role").as_string();
                             return c;
                         } catch (...) {
                             return std::nullopt;
                         }
                     }

                     std::optional<std::string> bearer_from(const std::string& header) {
                         const std::string p = "Bearer ";
                         if (header.size() <= p.size() || header.compare(0, p.size(), p) != 0)
                             return std::nullopt;
                         return header.substr(p.size());
                     }
