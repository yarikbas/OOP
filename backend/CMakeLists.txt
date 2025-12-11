cmake_minimum_required(VERSION 3.21)
project(oop_backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- deps for core/server ----
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(Drogon CONFIG REQUIRED)

# Якщо реально десь не використовуєш nlohmann_json — прибери
find_package(nlohmann_json CONFIG REQUIRED)

# ---- core ----
add_library(oop_core STATIC
    src/db/Db.cpp
    src/repos/ShipsRepo.cpp
    src/repos/PortsRepo.cpp
    src/repos/PeopleRepo.cpp
    src/repos/ShipTypesRepo.cpp
    src/repos/CompaniesRepo.cpp
    src/repos/CrewRepo.cpp
    src/repos/CargoRepo.cpp
    src/repos/VoyageRecordsRepo.cpp
    src/repos/VoyageExpensesRepo.cpp
    src/repos/SchedulesRepo.cpp
    src/repos/WeatherDataRepo.cpp
)

target_include_directories(oop_core PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(oop_core PUBLIC
    unofficial::sqlite3::sqlite3
    nlohmann_json::nlohmann_json
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
    src/controllers/LogsController.cpp
    src/controllers/CargoController.cpp
    src/controllers/VoyageRecordsController.cpp
    src/controllers/SchedulesController.cpp
    src/controllers/WeatherController.cpp
)

target_include_directories(oop_backend PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(oop_backend PRIVATE
    Drogon::Drogon
    oop_core
)
