find_package(Qt6 REQUIRED COMPONENTS Test Core)
find_package(OpenSSL REQUIRED)

add_executable(crypto_engine_test
    CryptoEngineTest.cpp
    ../src/AesGcmStrategy.cpp
    ../src/RsaOaepStrategy.cpp
    ../src/OpensslAdapter.cpp
)
set_target_properties(crypto_engine_test PROPERTIES AUTOMOC ON)
target_include_directories(crypto_engine_test PRIVATE ../src)
target_link_libraries(crypto_engine_test PRIVATE
    Qt6::Test Qt6::Core
    OpenSSL::SSL OpenSSL::Crypto
    common
)
add_test(NAME CryptoEngineTest COMMAND crypto_engine_test)
