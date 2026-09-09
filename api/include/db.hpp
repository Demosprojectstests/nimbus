#pragma once
#include <optional>
#include <string>
#include <vector>
#include <sqlite3.h>

struct User {
    int id{};
    std::string username;
    std::string role;
    std::string password_hash;
    std::string salt;
};

struct Record {
    int id{};
    int owner_id{};
    std::string title;
    std::string body;
};

class Database {
public:
    explicit Database(const std::string& path);
    ~Database();

    void migrate();
    bool create_user(const std::string& username, const std::string& hash,
                     const std::string& salt, const std::string& role);
    std::optional<User> find_user(const std::string& username);

    std::optional<Record> create_record(int owner_id, const std::string& title, const std::string& body);
    std::vector<Record> list_records(int owner_id, bool all);
    std::optional<Record> get_record(int id);
    bool update_record(int id, int requester_id, bool is_admin,
                       const std::string& title, const std::string& body);
    bool delete_record(int id, int requester_id, bool is_admin);

private:
    sqlite3* db_{nullptr};
};
