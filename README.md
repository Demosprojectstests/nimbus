# Nimbus

C++ REST API + Rust desktop client. JWT/RBAC, Docker, and CI/CD security come next.

## Run the API

```bash
cd api
cmake -S . -B build
cmake --build build
./build/nimbus_api
