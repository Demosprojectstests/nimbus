#include "validation.hpp"

int main() {
    if (valid_username("ab")) return 1;
    if (!valid_username("alice")) return 1;
    if (valid_password("short")) return 1;
    if (!valid_password("Correct1!xx")) return 1;
    if (valid_title("")) return 1;
    if (!valid_title("notes")) return 1;
    return 0;
}
