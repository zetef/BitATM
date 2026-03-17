qt_add_executable(frontend
	src/main.cpp
	src/NetworkManager.h
	src/NetworkManager.cpp
)
set_target_properties(frontend PROPERTIES AUTOMOC ON)

## see https://doc.qt.io/qt-6/qt-policy.html
if(QT_KNOWN_POLICY_QTP0001)
    qt_policy(SET QTP0001 NEW)
endif()
if(QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif()
qt_add_qml_module(frontend
	URI BitATM
	VERSION 1.0
	QML_FILES qml/Main.qml
)

target_link_libraries(frontend PRIVATE
	common
	Qt6::Core Qt6::Quick Qt6::Network Qt6::WebSockets
	OpenSSL::SSL OpenSSL::Crypto
)
target_include_directories(frontend PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_subdirectory(tests)
