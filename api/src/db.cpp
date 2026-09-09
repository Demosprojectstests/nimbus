#include "db.hpp"
#include <stdexcept>

Database::Database(const std::string& path) {
    if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK)
        throw std::runtime_error("sqlite open failed");
    sqlite3_exec(db_, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
}

Database::~Database() {
    if (db_) sqlite3_close(db_);
}

void Database::migrate() {
    char* err = nullptr;
    const char* users =
    "CREATE TABLE IF NOT EXISTS users ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "username TEXT NOT NULL UNIQUE,"
    "password_hash TEXT NOT NULL,"
    "salt TEXT NOT NULL,"
    "role TEXT NOT NULL CHECK(role IN ('admin','user'))"
    ");";
    if (sqlite3_exec(db_, users, nullptr, nullptr, &err) != SQLITE_OK) {
        std::string m = err ? err : "migrate failed";
        sqlite3_free(err);
        throw std::runtime_error(m);
    }

    const char* records =
    "CREATE TABLE IF NOT EXISTS records ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "owner_id INTEGER NOT NULL,"
    "title TEXT NOT NULL,"
    "body TEXT NOT NULL,"
    "FOREIGN KEY(owner_id) REFERENCES users(id)"
    ");";
    if (sqlite3_exec(db_, records, nullptr, nullptr, &err) != SQLITE_OK) {
        std::string m = err ? err : "migrate failed";
        sqlite3_free(err);
        throw std::runtime_error(m);
    }
}

bool Database::create_user(const std::string& username, const std::string& hash,
                           const std::string& salt, const std::string& role) {
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(db_,
                       "INSERT INTO users(username,password_hash,salt,role) VALUES(?1,?2,?3,?4)",
                       -1, &st, nullptr);
    sqlite3_bind_text(st, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 2, hash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 3, salt.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 4, role.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(st);
    sqlite3_finalize(st);
    return rc == SQLITE_DONE;
                           }

                           std::optional<User> Database::find_user(const std::string& username) {
                               sqlite3_stmt* st = nullptr;
                               sqlite3_prepare_v2(db_,
                                                  "SELECT id,username,password_hash,salt,role FROM users WHERE username=?1",
                                                  -1, &st, nullptr);
                               sqlite3_bind_text(st, 1, username.c_str(), -1, SQLITE_TRANSIENT);
                               std::optional<User> out;
                               if (sqlite3_step(st) == SQLITE_ROW) {
                                   User u;
                                   u.id = sqlite3_column_int(st, 0);
                                   u.username = reinterpret_cast<const char*>(sqlite3_column_text(st, 1));
                                   u.password_hash = reinterpret_cast<const char*>(sqlite3_column_text(st, 2));
                                   u.salt = reinterpret_cast<const char*>(sqlite3_column_text(st, 3));
                                   u.role = reinterpret_cast<const char*>(sqlite3_column_text(st, 4));
                                   out = u;
                               }
                               sqlite3_finalize(st);
                               return out;
                           }

                           std::optional<Record> Database::create_record(int owner_id, const std::string& title, const std::string& body) {
                               sqlite3_stmt* st = nullptr;
                               sqlite3_prepare_v2(db_,
                                                  "INSERT INTO records(owner_id,title,body) VALUES(?1,?2,?3)",
                                                  -1, &st, nullptr);
                               sqlite3_bind_int(st, 1, owner_id);
                               sqlite3_bind_text(st, 2, title.c_str(), -1, SQLITE_TRANSIENT);
                               sqlite3_bind_text(st, 3, body.c_str(), -1, SQLITE_TRANSIENT);
                               if (sqlite3_step(st) != SQLITE_DONE) {
                                   sqlite3_finalize(st);
                                   return std::nullopt;
                               }
                               sqlite3_finalize(st);

                               Record r;
                               r.id = static_cast<int>(sqlite3_last_insert_rowid(db_));
                               r.owner_id = owner_id;
                               r.title = title;
                               r.body = body;
                               return r;
                           }

                           std::vector<Record> Database::list_records(int owner_id, bool all) {
                               sqlite3_stmt* st = nullptr;
                               if (all) {
                                   sqlite3_prepare_v2(db_, "SELECT id,owner_id,title,body FROM records", -1, &st, nullptr);
                               } else {
                                   sqlite3_prepare_v2(db_,
                                                      "SELECT id,owner_id,title,body FROM records WHERE owner_id=?1",
                                                      -1, &st, nullptr);
                                   sqlite3_bind_int(st, 1, owner_id);
                               }

                               std::vector<Record> out;
                               while (sqlite3_step(st) == SQLITE_ROW) {
                                   Record r;
                                   r.id = sqlite3_column_int(st, 0);
                                   r.owner_id = sqlite3_column_int(st, 1);
                                   r.title = reinterpret_cast<const char*>(sqlite3_column_text(st, 2));
                                   r.body = reinterpret_cast<const char*>(sqlite3_column_text(st, 3));
                                   out.push_back(r);
                               }
                               sqlite3_finalize(st);
                               return out;
                           }

                           std::optional<Record> Database::get_record(int id) {
                               sqlite3_stmt* st = nullptr;
                               sqlite3_prepare_v2(db_,
                                                  "SELECT id,owner_id,title,body FROM records WHERE id=?1",
                                                  -1, &st, nullptr);
                               sqlite3_bind_int(st, 1, id);
                               std::optional<Record> out;
                               if (sqlite3_step(st) == SQLITE_ROW) {
                                   Record r;
                                   r.id = sqlite3_column_int(st, 0);
                                   r.owner_id = sqlite3_column_int(st, 1);
                                   r.title = reinterpret_cast<const char*>(sqlite3_column_text(st, 2));
                                   r.body = reinterpret_cast<const char*>(sqlite3_column_text(st, 3));
                                   out = r;
                               }
                               sqlite3_finalize(st);
                               return out;
                           }

                           bool Database::update_record(int id, int requester_id, bool is_admin,
                                                        const std::string& title, const std::string& body) {
                               sqlite3_stmt* st = nullptr;
                               if (is_admin) {
                                   sqlite3_prepare_v2(db_,
                                                      "UPDATE records SET title=?1, body=?2 WHERE id=?3",
                                                      -1, &st, nullptr);
                               } else {
                                   sqlite3_prepare_v2(db_,
                                                      "UPDATE records SET title=?1, body=?2 WHERE id=?3 AND owner_id=?4",
                                                      -1, &st, nullptr);
                               }
                               sqlite3_bind_text(st, 1, title.c_str(), -1, SQLITE_TRANSIENT);
                               sqlite3_bind_text(st, 2, body.c_str(), -1, SQLITE_TRANSIENT);
                               sqlite3_bind_int(st, 3, id);
                               if (!is_admin) sqlite3_bind_int(st, 4, requester_id);
                               sqlite3_step(st);
                               bool ok = sqlite3_changes(db_) > 0;
                               sqlite3_finalize(st);
                               return ok;
                                                        }

                                                        bool Database::delete_record(int id, int requester_id, bool is_admin) {
                                                            sqlite3_stmt* st = nullptr;
                                                            if (is_admin) {
                                                                sqlite3_prepare_v2(db_, "DELETE FROM records WHERE id=?1", -1, &st, nullptr);
                                                            } else {
                                                                sqlite3_prepare_v2(db_,
                                                                                   "DELETE FROM records WHERE id=?1 AND owner_id=?2",
                                                                                   -1, &st, nullptr);
                                                            }
                                                            sqlite3_bind_int(st, 1, id);
                                                            if (!is_admin) sqlite3_bind_int(st, 2, requester_id);
                                                            sqlite3_step(st);
                                                            bool ok = sqlite3_changes(db_) > 0;
                                                            sqlite3_finalize(st);
                                                            return ok;
                                                        }
