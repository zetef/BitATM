add_executable(bitatm-backend src/main.cpp)
target_link_libraries(bitatm-backend PRIVATE
    common
    Poco::Net
    Poco::Util
    Poco::Data
    Poco::DataPostgreSQL
    OpenSSL::SSL
    OpenSSL::Crypto
)
target_include_directories(bitatm-backend PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_subdirectory(tests)