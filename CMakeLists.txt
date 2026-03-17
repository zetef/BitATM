cmake_minimum_required(VERSION 3.25)
project(BitATM CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Quick Network WebSockets Test)
find_package(PostgreSQL REQUIRED)
set(CMAKE_MESSAGE_LOG_LEVEL ERROR) ## suppress vcpkg warnings
find_package(Poco REQUIRED COMPONENTS Net Data DataPostgreSQL Util)
set(CMAKE_MESSAGE_LOG_LEVEL STATUS) ## suppress vcpkg warnings
find_package(OpenSSL REQUIRED)

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
add_subdirectory(backend)
add_subdirectory(frontend)