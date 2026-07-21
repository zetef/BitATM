cmake_minimum_required(VERSION 3.25)
project(BitATM CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Quick Network WebSockets Test Sql)

# The backend (and its Poco/PostgreSQL deps, resolved via the desktop vcpkg
# triplet) never runs on Android - only the frontend gets cross-compiled for
# a phone, and vcpkg isn't wired into the NDK toolchain configure at all.
if(NOT ANDROID)
    find_package(PostgreSQL REQUIRED)
    set(CMAKE_MESSAGE_LOG_LEVEL ERROR) ## suppress vcpkg warnings
    find_package(Poco REQUIRED COMPONENTS Net Data DataPostgreSQL Util)
    set(CMAKE_MESSAGE_LOG_LEVEL STATUS) ## suppress vcpkg warnings
    find_package(OpenSSL REQUIRED)
endif()

### DOXYGEN
find_package(Doxygen OPTIONAL_COMPONENTS dot)

if(DOXYGEN_FOUND)
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating BitATM documentation → docs/generated/html/"
        VERBATIM
    )
endif()

enable_testing()

add_subdirectory(common)
if(NOT ANDROID)
    add_subdirectory(backend)
endif()
add_subdirectory(frontend)