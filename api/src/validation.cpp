#include "validation.hpp"
#include <cctype>
#include <regex>

bool valid_username(const std::string& s) {
    static const std::regex re(R"(^[A-Za-z0-9_]{3,32}$)");
    return std::regex_match(s, re);
}

bool valid_password(const std::string& s) {
    if (s.size() < 10 || s.size() > 128) return false;
    bool up = false, lo = false, di = false, sp = false;
    for (unsigned char c : s) {
        if (std::isupper(c)) up = true;
        else if (std::islower(c)) lo = true;
        else if (std::isdigit(c)) di = true;
        else sp = true;
    }
    return up && lo && di && sp;
}

bool valid_title(const std::string& s) {
    return !s.empty() && s.size() <= 120;
}
