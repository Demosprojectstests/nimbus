# Nimbus

C++ REST API + Rust CLI for users and owner-scoped records.

JWT auth, role claims, Docker (non-root), and GitHub Actions SAST + Trivy are in place.
AWS App Runner and Windows credential storage are the remaining brief items.

## API

C++17, Crow v1.3.2, SQLite prepared statements, jwt-cpp HS256, PBKDF2-SHA256.

| Method | Path | Auth |
| --- | --- | --- |
| GET | `/health` | no |
| POST | `/api/v1/auth/register` | no |
| POST | `/api/v1/auth/login` | no |
| GET | `/api/v1/me` | Bearer |
| GET | `/api/v1/records` | Bearer (owner; admin sees all) |
| POST | `/api/v1/records` | Bearer |
| DELETE | `/api/v1/records/:id` | Bearer (owner or admin) |

Roles: `user` / `admin`. Registration creates `user` only. Admin paths exist in code; no seed admin yet.

Password policy: 10–128 chars, upper, lower, digit, symbol. Example: `Correct1!xx`

```bash
cd api
cmake -S . -B build && cmake --build build
./build/nimbus_api
