add_executable(bitatm-backend
    src/main.cpp
    src/User.h                    src/User.cpp
    src/Message.h                 src/Message.cpp
    src/Session.h                 src/Session.cpp
    src/OfflineMessage.h          src/OfflineMessage.cpp
    src/ConversationHistory.h     src/ConversationHistory.cpp
    src/GroupMessage.h            src/GroupMessage.cpp
    src/ProtocolParser.h          src/ProtocolParser.cpp
    src/IRepository.h
    src/DbManager.h               src/DbManager.cpp
    src/UserRepository.h          src/UserRepository.cpp
    src/MessageRepository.h       src/MessageRepository.cpp
    src/SessionRepository.h       src/SessionRepository.cpp
    src/OfflineQueueRepository.h  src/OfflineQueueRepository.cpp
    src/ClientSession.h           src/ClientSession.cpp
    src/PacketHandlerFactory.h    src/PacketHandlerFactory.cpp
    src/Server.h                  src/Server.cpp
    src/ICommandHandler.h
    src/BaseAuthHandler.h         src/BaseAuthHandler.cpp
    src/RegisterHandler.h         src/RegisterHandler.cpp
    src/LoginHandler.h            src/LoginHandler.cpp
    src/MessageHandler.h          src/MessageHandler.cpp
    src/KeyExchangeHandler.h      src/KeyExchangeHandler.cpp
    src/AckHandler.h              src/AckHandler.cpp
)
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
