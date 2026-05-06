import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    visible: true
    width: 900
    height: 600
    title: "BitATM"
    color: "#1e1e2e"

    property string activePeer: ""
    property bool isMobile: root.width < 600

    Rectangle {
        id: errorBanner
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: visible ? 36 : 0
        color: networkManager.hasError ? "#f44336" : "#313244"
        z: 10
        visible: networkManager.lastMessage.length > 0

        Label {
            anchors.centerIn: parent
            text: networkManager.lastMessage
            color: "#ffffff"
            font.pixelSize: 13
        }

        Timer {
            id: bannerTimer
            interval: 4000
            onTriggered: errorBanner.visible = false
        }

        onVisibleChanged: {
            if (visible) bannerTimer.restart()
        }
    }

    Connections {
        target: networkManager

        function onCurrentUsernameChanged() {
            if (networkManager.currentUsername.length > 0) {
                stack.push(chatPageComponent)
            } else if (stack.depth > 1) {
                chatModel.clearAll()
                convListModel.clear()
                root.activePeer = ""
                stack.pop()
            }
        }

        function onMessageDecrypted(from, plaintext, timestamp) {
            chatModel.appendAndCache(from, from, plaintext, timestamp, false)
            convListModel.addOrUpdate(from, plaintext, timestamp)
            if (from === root.activePeer) {
                networkManager.markConversationRead(from)
            }
        }

        function onDisconnected() {
            chatModel.clearAll()
            convListModel.clear()
            root.activePeer = ""
            if (stack.depth > 1) stack.pop()
        }

        function onHistorySyncMessage(peer, sender, content, timestamp, isOutgoing) {
            chatModel.appendAndCache(peer, sender, content, timestamp, isOutgoing)
            convListModel.addOrUpdate(peer, content, timestamp)
        }

        function onMessageDelivered(timestamp) {
            chatModel.updateStatus(root.activePeer, timestamp, "delivered")
        }

        function onMessageSeen(peer, timestamp) {
            chatModel.updateStatus(peer, timestamp, "seen")
        }
    }

    Component {
        id: loginPageComponent

        Rectangle {
            color: "#1e1e2e"

            Column {
                anchors.centerIn: parent
                spacing: 14
                width: 300

                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "BitATM"
                    font.pixelSize: 28
                    font.bold: true
                    color: "#cdd6f4"
                }

                TextField {
                    id: usernameInput
                    placeholderText: "Username"
                    width: parent.width
                    color: "#cdd6f4"
                    placeholderTextColor: "#6c7086"
                    background: Rectangle { color: "#313244"; radius: 4 }
                    padding: 10
                }

                Label {
                    width: parent.width
                    visible: usernameInput.text.length > 0
                    text: usernameInput.text.length >= 3 ? "Username ok" : "Min 3 characters required"
                    color: usernameInput.text.length >= 3 ? "#a6e3a1" : "#f38ba8"
                    font.pixelSize: 10
                }

                TextField {
                    id: passwordInput
                    placeholderText: "Password"
                    echoMode: TextInput.Password
                    width: parent.width
                    color: "#cdd6f4"
                    placeholderTextColor: "#6c7086"
                    background: Rectangle { color: "#313244"; radius: 4 }
                    padding: 10
                    onAccepted: {
                        if (usernameInput.text.length > 0 && passwordInput.text.length > 0)
                            networkManager.sendLogin(usernameInput.text, passwordInput.text)
                    }
                }

                Label {
                    width: parent.width
                    visible: passwordInput.text.length > 0
                    text: passwordInput.text.length >= 8 ? "Password ok" : "Min 8 characters for Register"
                    color: passwordInput.text.length >= 8 ? "#a6e3a1" : "#f38ba8"
                    font.pixelSize: 10
                }

                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 10

                    Button {
                        text: "Register"
                        enabled: networkManager.isConnected &&
                                 usernameInput.text.length >= 3 &&
                                 passwordInput.text.length >= 8
                        onClicked: networkManager.sendRegister(usernameInput.text, passwordInput.text)
                        contentItem: Text {
                            text: parent.text
                            color: "#cdd6f4"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: parent.enabled ? (parent.down ? "#45475a" : "#313244") : "#252535"
                            radius: 4
                        }
                    }

                    Button {
                        text: "Login"
                        enabled: networkManager.isConnected &&
                                 usernameInput.text.length > 0 &&
                                 passwordInput.text.length > 0
                        onClicked: networkManager.sendLogin(usernameInput.text, passwordInput.text)
                        contentItem: Text {
                            text: parent.text
                            color: "#1e1e2e"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: parent.enabled ? (parent.down ? "#74a0e8" : "#89b4fa") : "#45475a"
                            radius: 4
                        }
                    }
                }

                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 8

                    Rectangle {
                        width: 10
                        height: 10
                        radius: 5
                        anchors.verticalCenter: parent.verticalCenter
                        color: networkManager.isConnected ? "#a6e3a1" : "#f38ba8"
                    }

                    Label {
                        text: networkManager.isConnected ? "Connected" : "Disconnected"
                        color: networkManager.isConnected ? "#a6e3a1" : "#f38ba8"
                        font.pixelSize: 12
                    }
                }
            }
        }
    }

    Component {
        id: chatPageComponent

        Rectangle {
            color: "#1e1e2e"

            RowLayout {
                anchors.fill: parent
                spacing: 0
                visible: !root.isMobile

                ConversationListPage {
                    Layout.preferredWidth: 220
                    Layout.fillHeight: true
                    activePeer: root.activePeer
                    isMobile: root.isMobile
                    onPeerSelected: function(peer) { root.activePeer = peer }
                }

                Rectangle {
                    width: 1
                    Layout.fillHeight: true
                    color: "#313244"
                }

                ChatPage {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    activePeer: root.activePeer
                    isMobile: root.isMobile
                }
            }

            StackView {
                id: mobileStack
                anchors.fill: parent
                visible: root.isMobile

                initialItem: ConversationListPage {
                    activePeer: root.activePeer
                    isMobile: root.isMobile
                    stackView: mobileStack
                    chatPageComponent: mobileChatComponent
                    onPeerSelected: function(peer) { root.activePeer = peer }
                }
            }

            Component {
                id: mobileChatComponent
                ChatPage {
                    activePeer: root.activePeer
                    isMobile: root.isMobile
                    stackView: mobileStack
                }
            }
        }
    }

    StackView {
        id: stack
        anchors.top: errorBanner.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        initialItem: loginPageComponent
    }
}
