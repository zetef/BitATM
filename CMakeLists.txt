cmake_minimum_required(VERSION 3.25)
project(BitATM CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRES ON)

find_package(Qt6 REQUIRED COMPONENTS Core Quick Network WebSockets Test)
find_package(PostgreSQL REQUIRED)
find_package(Poco REQUIRED COMPONENTS Net Data DataPostgreSQL Util)
find_package(OpenSSL REQUIRED)

add_subdirectory(common)
add_subdirectory(backend)
add_subdirectory(frontend)
