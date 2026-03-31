add_executable(bitatm-backend src/main.cpp
    src/User.h src/User.cpp
    src/Message.h src/Message.cpp
    src/Session.h src/Session.cpp
    src/OfflineMessage.h src/OfflineMessage.cpp
    src/ConversationHistory.h src/ConversationHistory.cpp
    src/GroupMessage.h src/GroupMessage.cpp
    src/ProtocolParser.h src/ProtocolParser.cpp)
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
