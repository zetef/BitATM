find_package(Qt6 REQUIRED COMPONENTS Test)

# UT-BE-01, UT-BE-02 - DbManager + repositories (requires DATABASE_URL)
add_executable(db_manager_test DbManagerTest.cpp)
set_target_properties(db_manager_test PROPERTIES AUTOMOC ON)
target_link_libraries(db_manager_test PRIVATE Qt6::Test bitatm-backend-lib)
add_test(NAME DbManagerTest COMMAND db_manager_test)

# UT-BE-03, UT-BE-04 - ProtocolParser round-trip (pure logic, always runs)
add_executable(protocol_parser_test ProtocolParserTest.cpp)
set_target_properties(protocol_parser_test PROPERTIES AUTOMOC ON)
target_link_libraries(protocol_parser_test PRIVATE Qt6::Test bitatm-backend-lib)
add_test(NAME ProtocolParserTest COMMAND protocol_parser_test)

# UT-BE-05 - LoginHandler (requires server on localhost:8080)
add_executable(login_handler_test LoginHandlerTest.cpp)
set_target_properties(login_handler_test PROPERTIES AUTOMOC ON)
target_link_libraries(login_handler_test PRIVATE Qt6::Test bitatm-backend-lib)
add_test(NAME LoginHandlerTest COMMAND login_handler_test)

# UT-BE-06 - MessageHandler (requires server on localhost:8080)
add_executable(message_handler_test MessageHandlerTest.cpp)
set_target_properties(message_handler_test PROPERTIES AUTOMOC ON)
target_link_libraries(message_handler_test PRIVATE Qt6::Test bitatm-backend-lib)
add_test(NAME MessageHandlerTest COMMAND message_handler_test)

# UT-BE-07, UT-BE-08 - SyncHistoryHandler / MessageRepository multi-device
add_executable(sync_history_handler_test SyncHistoryHandlerTest.cpp)
set_target_properties(sync_history_handler_test PROPERTIES AUTOMOC ON)
target_link_libraries(sync_history_handler_test PRIVATE Qt6::Test bitatm-backend-lib)
add_test(NAME SyncHistoryHandlerTest COMMAND sync_history_handler_test)

# UT-BE-09..11 - Group handler tests
add_executable(create_group_handler_test CreateGroupHandlerTest.cpp)
set_target_properties(create_group_handler_test PROPERTIES AUTOMOC ON)
target_link_libraries(create_group_handler_test PRIVATE Qt6::Test bitatm-backend-lib)
add_test(NAME CreateGroupHandlerTest COMMAND create_group_handler_test)

# UT-BE-12 - OfflineQueueRepository retry cap (requires DATABASE_URL)
add_executable(offline_queue_repository_test OfflineQueueRepositoryTest.cpp)
set_target_properties(offline_queue_repository_test PROPERTIES AUTOMOC ON)
target_link_libraries(offline_queue_repository_test PRIVATE Qt6::Test bitatm-backend-lib)
add_test(NAME OfflineQueueRepositoryTest COMMAND offline_queue_repository_test)

# UT-BE-13 - SessionRepository token validation (requires DATABASE_URL)
add_executable(session_repository_test SessionRepositoryTest.cpp)
set_target_properties(session_repository_test PROPERTIES AUTOMOC ON)
target_link_libraries(session_repository_test PRIVATE Qt6::Test bitatm-backend-lib)
add_test(NAME SessionRepositoryTest COMMAND session_repository_test)

# Beyond DCS baseline - ConversationHistory operator[]/operator+ and
# Message operator> live call sites (pure logic, always runs)
add_executable(conversation_history_test ConversationHistoryTest.cpp)
set_target_properties(conversation_history_test PROPERTIES AUTOMOC ON)
target_link_libraries(conversation_history_test PRIVATE Qt6::Test bitatm-backend-lib)
add_test(NAME ConversationHistoryTest COMMAND conversation_history_test)

# UT-BE-14 - GroupRepository per-member receipts (requires DATABASE_URL)
add_executable(group_receipts_test GroupReceiptsTest.cpp)
set_target_properties(group_receipts_test PROPERTIES AUTOMOC ON)
target_link_libraries(group_receipts_test PRIVATE Qt6::Test bitatm-backend-lib)
add_test(NAME GroupReceiptsTest COMMAND group_receipts_test)

# Pending notification queue (offline delete/group-leave delivery on next login)
add_executable(pending_notification_repository_test PendingNotificationRepositoryTest.cpp)
set_target_properties(pending_notification_repository_test PROPERTIES AUTOMOC ON)
target_link_libraries(pending_notification_repository_test PRIVATE Qt6::Test bitatm-backend-lib)
add_test(NAME PendingNotificationRepositoryTest COMMAND pending_notification_repository_test)

# MessageRepository::deleteConversation ("delete for everyone" server-side cleanup)
add_executable(message_repository_test MessageRepositoryTest.cpp)
set_target_properties(message_repository_test PROPERTIES AUTOMOC ON)
target_link_libraries(message_repository_test PRIVATE Qt6::Test bitatm-backend-lib)
add_test(NAME MessageRepositoryTest COMMAND message_repository_test)

# DELETE_CONVERSATION: online forward + offline queue-then-flush-on-login
add_executable(delete_conversation_handler_test DeleteConversationHandlerTest.cpp)
set_target_properties(delete_conversation_handler_test PROPERTIES AUTOMOC ON)
target_link_libraries(delete_conversation_handler_test PRIVATE Qt6::Test bitatm-backend-lib)
add_test(NAME DeleteConversationHandlerTest COMMAND delete_conversation_handler_test)
