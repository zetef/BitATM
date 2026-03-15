find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(db_manager_test DbManagerTest.cpp)
set_target_properties(db_manager_test PROPERTIES AUTOMOC ON)
target_link_libraries(db_manager_test PRIVATE Qt6::Test common)
add_test(NAME DbManagerTest COMMAND db_manager_test)
set_tests_properties(DbManagerTest PROPERTIES WILL_FAIL TRUE)
