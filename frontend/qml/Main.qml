import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    visible: true
    width: 900
    height: 600
    title: "BitATM"
    color: "#0a0a0a"

    property string activePeer: ""
    property bool isMobile: root.width < 600

    Rectangle {
        id: errorBanner
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: visible ? 36 : 0
        color: networkManager.hasError ? "#ff3333" : "#1a1a1a"
        z: 10
        visible: networkManager.lastMessage.length > 0

        Label {
            anchors.centerIn: parent
            text: networkManager.lastMessage
            color: "#c8c8c8"
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

        function onConvListUpdated(peer, lastMessage, lastTimestamp) {
            convListModel.addOrUpdate(peer, lastMessage, lastTimestamp)
        }

        function onGroupInviteReceived(groupId, groupName) {
            convListModel.addOrUpdateGroup(groupId, groupName, "Group invite received", "")
        }

        function onGroupMessageDecrypted(groupId, sender, plaintext, timestamp, isOutgoing) {
            chatModel.appendAndCache(groupId, sender, plaintext, timestamp, isOutgoing)
            convListModel.addOrUpdateGroup(groupId, "", plaintext, timestamp)
        }

        function onGroupLeft(groupId) {
            if (root.activePeer === groupId) root.activePeer = ""
            convListModel.remove(groupId)
        }

        function onGroupHistorySyncMessage(groupId, sender, content, timestamp, isOutgoing) {
            chatModel.appendAndCache(groupId, sender, content, timestamp, isOutgoing)
        }

        function onGroupConvUpdated(groupId, groupName, lastMessage, lastTimestamp) {
            convListModel.addOrUpdateGroup(groupId, groupName, lastMessage, lastTimestamp)
        }

        function onGroupInfoReceived(groupId, groupName, members) {
            convListModel.addOrUpdateGroup(groupId, groupName, "", "")
        }
    }

    Component {
        id: loginPageComponent

        Rectangle {
            color: "#0a0a0a"

            Column {
                anchors.centerIn: parent
                spacing: 14
                width: 300

                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "BitATM v1.0"
                    font.pixelSize: 28
                    font.bold: true
                    font.family: "Monospace"
                    color: "#00ff41"
                }

                TextField {
                    id: usernameInput
                    placeholderText: "> username"
                    width: parent.width
                    color: "#c8c8c8"
                    placeholderTextColor: "#505050"
                    font.family: "Monospace"
                    background: Rectangle { color: "#1a1a1a"; radius: 0 }
                    padding: 10
                }

                Label {
                    width: parent.width
                    visible: usernameInput.text.length > 0
                    text: usernameInput.text.length >= 3 ? "Username ok" : "Min 3 characters required"
                    color: usernameInput.text.length >= 3 ? "#00ff41" : "#ff3333"
                    font.pixelSize: 10
                    font.family: "Monospace"
                }

                TextField {
                    id: passwordInput
                    placeholderText: "> password"
                    echoMode: TextInput.Password
                    width: parent.width
                    color: "#c8c8c8"
                    placeholderTextColor: "#505050"
                    font.family: "Monospace"
                    background: Rectangle { color: "#1a1a1a"; radius: 0 }
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
                    color: passwordInput.text.length >= 8 ? "#00ff41" : "#ff3333"
                    font.pixelSize: 10
                    font.family: "Monospace"
                }

                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 10

                    Button {
                        text: "register"
                        enabled: networkManager.isConnected &&
                                 usernameInput.text.length >= 3 &&
                                 passwordInput.text.length >= 8
                        onClicked: networkManager.sendRegister(usernameInput.text, passwordInput.text)
                        contentItem: Text {
                            text: parent.text
                            color: "#c8c8c8"
                            font.family: "Monospace"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: parent.enabled ? (parent.down ? "#222222" : "#1a1a1a") : "#111111"
                            radius: 0
                            border.color: "#404040"
                            border.width: 1
                        }
                    }

                    Button {
                        text: ">> login"
                        enabled: networkManager.isConnected &&
                                 usernameInput.text.length > 0 &&
                                 passwordInput.text.length > 0
                        onClicked: networkManager.sendLogin(usernameInput.text, passwordInput.text)
                        contentItem: Text {
                            text: parent.text
                            color: "#0a0a0a"
                            font.family: "Monospace"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: parent.enabled ? (parent.down ? "#00cc33" : "#00ff41") : "#222222"
                            radius: 0
                        }
                    }
                }

                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: networkManager.isConnected ? "[ONLINE]" : "[OFFLINE]"
                    color: networkManager.isConnected ? "#00ff41" : "#ff3333"
                    font.pixelSize: 12
                    font.family: "Monospace"
                }
            }
        }
    }

    Component {
        id: chatPageComponent

        Rectangle {
            color: "#0a0a0a"

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
                    color: "#1a1a1a"
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
