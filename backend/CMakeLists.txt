add_executable(bitatm-backend src/main.cpp)
target_link_libraries(bitatm-backend PRIVATE
    common
    Poco::Net
    Poco::Data
    Poco::DataPostgreSQL
    OpenSSL::SSL
    OpenSSL::Crypto
)
target_include_directories(bitatm-backend PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

enable_testing()
add_subdirectory(tests)