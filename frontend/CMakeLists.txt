if(ANDROID)
    # Defines the OpenSSL::SSL / OpenSSL::Crypto imported targets from the
    # project's prebuilt Android OpenSSL libs (must run before the
    # target_link_libraries() call below references them - vcpkg's desktop
    # OpenSSL from the top-level find_package() doesn't apply to this toolchain).
    include("$ENV{HOME}/android_openssl/android_openssl.cmake")
endif()

qt_add_executable(frontend
	src/main.cpp
	src/NetworkManager.h
	src/NetworkManager.cpp
	src/LocalStorage.h
	src/LocalStorage.cpp
	src/ISymmetricStrategy.h
	src/IAsymmetricStrategy.h
	src/OpensslAdapter.h
	src/OpensslAdapter.cpp
	src/PacketBuilder.h
	src/PacketBuilder.cpp
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
	QML_FILES
		qml/Main.qml
		qml/ConversationListPage.qml
		qml/ChatPage.qml
		qml/CreateGroupDialog.qml
		qml/GroupInfoSheet.qml
		qml/MessageInfoSheet.qml
	SOURCES src/AesGcmStrategy.h
	SOURCES src/AesGcmStrategy.cpp
	SOURCES src/RsaOaepStrategy.h
	SOURCES src/RsaOaepStrategy.cpp
	SOURCES src/CryptoEngine.h
	SOURCES src/CryptoEngine.cpp
	SOURCES src/ChatModel.h
	SOURCES src/ChatModel.cpp
	SOURCES src/ConversationListModel.h
	SOURCES src/ConversationListModel.cpp
)

if(NOT DEFINED BITATM_SERVER_URL)
    set(BITATM_SERVER_URL "wss://api.zetef.xyz")
endif()
target_compile_definitions(frontend PRIVATE BITATM_SERVER_URL="${BITATM_SERVER_URL}")

target_link_libraries(frontend PRIVATE
	common
	Qt6::Core Qt6::Quick Qt6::Network Qt6::WebSockets Qt6::Sql
	OpenSSL::SSL OpenSSL::Crypto
)
target_include_directories(frontend PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(ANDROID)
    # Links OpenSSL::SSL/Crypto (targets defined by the include() above) and
    # sets QT_ANDROID_EXTRA_LIBS with the correct debug/release (no-asm vs
    # regular) and ABI-specific .so paths - replaces a previous hand-rolled
    # set_property() that hardcoded the ssl_3 path and didn't account for
    # the no-asm Debug build variant the script switches on.
    add_android_openssl_libraries(frontend)

    # Custom manifest sets android:windowSoftInputMode="adjustResize" so the
    # OS actually shrinks the app's visible viewport when the on-screen
    # keyboard appears - without it, bottom-anchored content (dialogs,
    # drawers, the chat input) gets covered by the keyboard with no way for
    # QML-side layout alone to reliably work around it.
    set_property(TARGET frontend PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/android")
endif()

# Unit tests are desktop-only (CI targets ubuntu-latest/windows-latest);
# they're never meant to build/run on an Android device.
if(NOT ANDROID)
    add_subdirectory(tests)
endif()
