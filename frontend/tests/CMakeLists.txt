find_package(Qt6 REQUIRED COMPONENTS Test Core Sql)
find_package(OpenSSL REQUIRED)

# UT-FE-01..04: CryptoEngine (Facade) tests
add_executable(crypto_engine_test
    CryptoEngineTest.cpp
    ../src/CryptoEngine.cpp
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

# UT-FE-07..08: PacketBuilder tests
add_executable(packet_builder_test
    PacketBuilderTest.cpp
    ../src/PacketBuilder.cpp
)
set_target_properties(packet_builder_test PROPERTIES AUTOMOC ON)
target_include_directories(packet_builder_test PRIVATE ../src)
target_link_libraries(packet_builder_test PRIVATE
    Qt6::Test Qt6::Core
    common
)
add_test(NAME PacketBuilderTest COMMAND packet_builder_test)

# UT-FE-05..06: ChatModel tests
add_executable(chat_model_test
    ChatModelTest.cpp
    ../src/ChatModel.cpp
)
set_target_properties(chat_model_test PROPERTIES AUTOMOC ON)
target_include_directories(chat_model_test PRIVATE ../src)
target_link_libraries(chat_model_test PRIVATE
    Qt6::Test Qt6::Core
    common
)
add_test(NAME ChatModelTest COMMAND chat_model_test)

# UT-FE-12, UT-FE-13 - LocalStorage SQLite cache
add_executable(local_storage_test
    LocalStorageTest.cpp
    ../src/LocalStorage.cpp
)
set_target_properties(local_storage_test PROPERTIES AUTOMOC ON)
target_include_directories(local_storage_test PRIVATE ../src)
target_link_libraries(local_storage_test PRIVATE
    Qt6::Test Qt6::Core Qt6::Sql
)
add_test(NAME LocalStorageTest COMMAND local_storage_test)
