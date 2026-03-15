add_executable(backend src/main.cpp)
target_link_libraries(backend PRIVATE
	common
	Poco::Net
	Poco::Util
	Poco::Data
	Poco::DataPostgreSQL
	OpenSSL::SSL
	OpenSSL::Crypto
)
target_include_directories(backend PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

enable_testing()
add_subdirectory(tests)
