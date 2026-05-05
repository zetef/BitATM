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

    // Error/status banner
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
                stack.push(chatPage)
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

    StackView {
        id: stack
        anchors.top: errorBanner.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        initialItem: loginPage
    }

    // Login page
    Component {
        id: loginPage

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

    // Chat page
    Component {
        id: chatPage

        Rectangle {
            color: "#1e1e2e"

            RowLayout {
                anchors.fill: parent
                spacing: 0

                // Sidebar
                Rectangle {
                    width: 220
                    Layout.fillHeight: true
                    color: "#181825"

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        // Sidebar header - username + logout
                        Rectangle {
                            Layout.fillWidth: true
                            height: 48
                            color: "#11111b"

                            Label {
                                anchors.left: parent.left
                                anchors.leftMargin: 12
                                anchors.verticalCenter: parent.verticalCenter
                                text: networkManager.currentUsername
                                color: "#cdd6f4"
                                font.pixelSize: 14
                                font.bold: true
                            }

                            Button {
                                anchors.right: parent.right
                                anchors.rightMargin: 8
                                anchors.verticalCenter: parent.verticalCenter
                                text: "Log out"
                                font.pixelSize: 11
                                onClicked: networkManager.logout()
                                contentItem: Text {
                                    text: parent.text
                                    color: "#f38ba8"
                                    font.pixelSize: 11
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                background: Rectangle {
                                    color: parent.down ? "#3b1e1e" : "transparent"
                                    radius: 4
                                    border.color: "#f38ba8"
                                    border.width: 1
                                }
                            }
                        }

                        // New chat input row
                        Row {
                            Layout.fillWidth: true
                            Layout.margins: 8
                            spacing: 6

                            TextField {
                                id: newChatInput
                                width: parent.width - goButton.width - parent.spacing
                                placeholderText: "Start chat..."
                                color: "#cdd6f4"
                                placeholderTextColor: "#585b70"
                                font.pixelSize: 12
                                background: Rectangle { color: "#313244"; radius: 4 }
                                padding: 8
                                onAccepted: {
                                    if (newChatInput.text.length > 0) {
                                        root.activePeer = newChatInput.text
                                        chatModel.switchConversation(newChatInput.text)
                                        networkManager.markConversationRead(newChatInput.text)
                                        networkManager.fetchPeerKey(newChatInput.text)
                                        newChatInput.text = ""
                                    }
                                }
                            }

                            Button {
                                id: goButton
                                text: "Go"
                                width: 36
                                height: newChatInput.height
                                onClicked: {
                                    if (newChatInput.text.length > 0) {
                                        root.activePeer = newChatInput.text
                                        chatModel.switchConversation(newChatInput.text)
                                        networkManager.markConversationRead(newChatInput.text)
                                        networkManager.fetchPeerKey(newChatInput.text)
                                        newChatInput.text = ""
                                    }
                                }
                                contentItem: Text {
                                    text: parent.text
                                    color: "#cdd6f4"
                                    font.pixelSize: 12
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                background: Rectangle {
                                    color: parent.down ? "#45475a" : "#89b4fa"
                                    radius: 4
                                }
                            }
                        }

                        // Conversation list
                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            model: convListModel
                            clip: true

                            delegate: ItemDelegate {
                                width: ListView.view.width
                                height: 60
                                highlighted: root.activePeer === model.username

                                background: Rectangle {
                                    color: parent.highlighted ? "#313244" : (parent.hovered ? "#252535" : "transparent")
                                }

                                Column {
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 12
                                    spacing: 2

                                    Label {
                                        width: parent.width
                                        text: model.username
                                        color: "#cdd6f4"
                                        font.pixelSize: 13
                                        font.bold: true
                                        elide: Text.ElideRight
                                    }

                                    Label {
                                        width: parent.width
                                        text: model.lastMessage
                                        color: "#6c7086"
                                        font.pixelSize: 11
                                        elide: Text.ElideRight
                                    }
                                }

                                onClicked: {
                                    root.activePeer = model.username
                                    chatModel.switchConversation(model.username)
                                    networkManager.markConversationRead(model.username)
                                }
                            }
                        }
                    }
                }

                // Divider
                Rectangle {
                    width: 1
                    Layout.fillHeight: true
                    color: "#313244"
                }

                // Chat area - welcome screen or active conversation
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    // Welcome screen: shown when no conversation is selected
                    Column {
                        visible: root.activePeer.length === 0
                        anchors.centerIn: parent
                        spacing: 12

                        Label {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "Welcome to BitATM"
                            color: "#cdd6f4"
                            font.pixelSize: 22
                            font.bold: true
                        }

                        Label {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "Select a conversation or start a new one"
                            color: "#6c7086"
                            font.pixelSize: 14
                        }
                    }

                    // Active conversation view
                    ColumnLayout {
                        visible: root.activePeer.length > 0
                        anchors.fill: parent
                        spacing: 0

                        // Chat header
                        Rectangle {
                            Layout.fillWidth: true
                            height: 48
                            color: "#181825"

                            Label {
                                anchors.centerIn: parent
                                text: root.activePeer
                                color: "#cdd6f4"
                                font.pixelSize: 14
                                font.bold: true
                            }
                        }

                        // Message list - newest at bottom
                        ListView {
                            id: messageList
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            model: chatModel
                            clip: true
                            spacing: 4
                            onCountChanged: Qt.callLater(positionViewAtEnd)

                            topMargin: 8
                            bottomMargin: 8
                            leftMargin: 12
                            rightMargin: 12

                            delegate: Item {
                                width: messageList.width - messageList.leftMargin - messageList.rightMargin
                                height: bubbleCol.implicitHeight + 6

                                Column {
                                    id: bubbleCol
                                    width: parent.width
                                    spacing: 2

                                    Item {
                                        width: parent.width
                                        height: bubble.height

                                        Rectangle {
                                            id: bubble
                                            width: Math.min(bubbleLabel.implicitWidth + 24, parent.width * 0.72)
                                            height: bubbleLabel.implicitHeight + 14
                                            radius: 12
                                            color: model.isOutgoing ? "#89b4fa" : "#313244"
                                            anchors.right: model.isOutgoing ? parent.right : undefined
                                            anchors.left:  model.isOutgoing ? undefined : parent.left

                                            Label {
                                                id: bubbleLabel
                                                anchors.left: parent.left
                                                anchors.right: parent.right
                                                anchors.top: parent.top
                                                anchors.margins: 12
                                                text: model.content
                                                color: model.isOutgoing ? "#1e1e2e" : "#cdd6f4"
                                                font.pixelSize: 13
                                                wrapMode: Text.Wrap
                                            }
                                        }
                                    }

                                    // HH:MM timestamp below bubble
                                    Label {
                                        width: parent.width
                                        text: {
                                            var ts = model.timestamp
                                            if (!ts || ts.length === 0) return ""
                                            var t = ts.indexOf("T")
                                            if (t >= 0 && ts.length > t + 5)
                                                return ts.substring(t + 1, t + 6)
                                            return ts.length >= 16 ? ts.substring(0, 16) : ts
                                        }
                                        color: "#585b70"
                                        font.pixelSize: 10
                                        horizontalAlignment: model.isOutgoing ? Text.AlignRight : Text.AlignLeft
                                    }

                                    // Status indicator for outgoing messages
                                    Label {
                                        id: statusLabel
                                        visible: model.isOutgoing
                                        width: parent.width
                                        text: model.status === "seen"      ? "✓✓"
                                            : model.status === "delivered" ? "✓✓"
                                            :                                "✓"
                                        // Blue only for "seen"; grey for sent/delivered
                                        color: model.status === "seen" ? "#89b4fa" : "#585b70"
                                        font.pixelSize: 10
                                        horizontalAlignment: Text.AlignRight

                                        HoverHandler { id: statusHover }

                                        ToolTip.visible: statusHover.hovered
                                        ToolTip.delay: 400
                                        ToolTip.text: model.status === "seen"      ? "Seen"
                                                    : model.status === "delivered" ? "Delivered"
                                                    :                                "Sent"
                                    }
                                }
                            }
                        }

                        // Input bar
                        Rectangle {
                            Layout.fillWidth: true
                            height: 56
                            color: "#181825"

                            Row {
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 8

                                TextField {
                                    id: msgInput
                                    width: parent.width - sendButton.width - parent.spacing
                                    height: parent.height
                                    placeholderText: "Message " + root.activePeer + "..."
                                    color: "#cdd6f4"
                                    placeholderTextColor: "#585b70"
                                    background: Rectangle { color: "#313244"; radius: 4 }
                                    padding: 10
                                    onAccepted: sendButton.clicked()
                                }

                                Button {
                                    id: sendButton
                                    text: "Send"
                                    width: 70
                                    height: parent.height
                                    enabled: msgInput.text.length > 0
                                    onClicked: {
                                        var txt = msgInput.text
                                        if (txt.length === 0) return
                                        msgInput.text = ""
                                        var ts = new Date().toISOString()
                                        chatModel.appendAndCache(root.activePeer,
                                                                networkManager.currentUsername,
                                                                txt, ts, true)
                                        convListModel.addOrUpdate(root.activePeer, txt, ts)
                                        networkManager.sendMessage(root.activePeer, txt, ts)
                                    }
                                    contentItem: Text {
                                        text: parent.text
                                        color: "#1e1e2e"
                                        font.pixelSize: 13
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    background: Rectangle {
                                        color: parent.enabled ? (parent.down ? "#74a0e8" : "#89b4fa") : "#45475a"
                                        radius: 4
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
