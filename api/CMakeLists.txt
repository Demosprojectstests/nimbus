cmake_minimum_required(VERSION 3.16)
project(nimbus_api LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(Crow
  GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
  GIT_TAG v1.3.2)
FetchContent_MakeAvailable(Crow)

FetchContent_Declare(jwt_cpp
  GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
  GIT_TAG v0.7.0)
set(JWT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(jwt_cpp)

find_package(OpenSSL REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)

add_executable(nimbus_api
  src/main.cpp
  src/auth.cpp
  src/db.cpp
  src/validation.cpp
)
target_include_directories(nimbus_api PRIVATE include)
target_link_libraries(nimbus_api PRIVATE
  Crow::Crow
  jwt-cpp::jwt-cpp
  OpenSSL::Crypto
  SQLite::SQLite3
  Threads::Threads
)

enable_testing()
add_executable(api_tests tests/test_validation.cpp src/validation.cpp)
target_include_directories(api_tests PRIVATE include)
add_test(NAME validation COMMAND api_tests)
