cmake_minimum_required(VERSION 3.21)
project(oop_backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(Drogon CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

# ---- core ----
add_library(oop_core STATIC
    src/db/Db.cpp
    src/repos/ShipsRepo.cpp
    src/repos/PortsRepo.cpp
    src/repos/PeopleRepo.cpp
    src/repos/ShipTypesRepo.cpp
    src/repos/CompaniesRepo.cpp
    src/repos/CrewRepo.cpp
)

target_include_directories(oop_core PUBLIC include)
target_link_libraries(oop_core PUBLIC
    nlohmann_json::nlohmann_json
    unofficial::sqlite3::sqlite3
)

# ---- server ----
add_executable(oop_backend
    src/main.cpp
    src/controllers/ShipsController.cpp
    src/controllers/PortsController.cpp
    src/controllers/PeopleController.cpp
    src/controllers/ShipTypesController.cpp
    src/controllers/CompaniesController.cpp
    src/controllers/CrewController.cpp
)

target_include_directories(oop_backend PRIVATE include)
target_link_libraries(oop_backend PRIVATE Drogon::Drogon oop_core)

add_executable(oop_tests
    tests/ShipsRepoTests.cpp
    tests/PeopleRepoTests.cpp
    tests/ShipTypesRepoTests.cpp
    # тимчасово вимикаємо, бо код змінився:
    # tests/PortsRepoTests.cpp
    # tests/CompaniesRepoTests.cpp
    tests/CrewRepoTests.cpp
)


target_include_directories(oop_tests PRIVATE include)
target_link_libraries(oop_tests PRIVATE
    oop_core
    GTest::gtest
    GTest::gtest_main
)

enable_testing()
add_test(NAME AllRepoTests COMMAND oop_tests)