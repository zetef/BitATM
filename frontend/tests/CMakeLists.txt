find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(crypto_engine_test CryptoEngineTest.cpp)
set_target_properties(crypto_engine_test PROPERTIES AUTOMOC ON)
target_link_libraries(crypto_engine_test PRIVATE Qt6::Test common)
add_test(NAME CryptoEngineTest COMMAND crypto_engine_test)
set_tests_properties(CryptoEngineTest PROPERTIES WILL_FAIL TRUE)
