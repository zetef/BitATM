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

    // Error banner
    Rectangle {
        id: errorBanner
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: visible ? 36 : 0
        color: "#f44336"
        z: 10
        visible: networkManager.hasError

        Label {
            anchors.centerIn: parent
            text: networkManager.lastMessage
            color: "#ffffff"
            font.pixelSize: 13
        }

        Timer {
            id: bannerTimer
            interval: 5000
            onTriggered: errorBanner.visible = false
        }

        onVisibleChanged: {
            if (visible) bannerTimer.restart()
        }
    }

    // Navigation to chat on login
    Connections {
        target: networkManager
        function onCurrentUsernameChanged() {
            if (networkManager.currentUsername.length > 0) {
                stack.push(chatPage)
            }
        }
        function onMessageDecrypted(from, plaintext, timestamp) {
            chatModel.appendMessage(from, plaintext, timestamp, false)
            convListModel.addOrUpdate(from, plaintext, timestamp)
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
                    background: Rectangle {
                        color: "#313244"
                        radius: 4
                    }
                    padding: 10
                }

                TextField {
                    id: passwordInput
                    placeholderText: "Password"
                    echoMode: TextInput.Password
                    width: parent.width
                    color: "#cdd6f4"
                    placeholderTextColor: "#6c7086"
                    background: Rectangle {
                        color: "#313244"
                        radius: 4
                    }
                    padding: 10
                    onAccepted: {
                        if (usernameInput.text.length > 0 && passwordInput.text.length > 0)
                            networkManager.sendLogin(usernameInput.text, passwordInput.text)
                    }
                }

                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 10

                    Button {
                        text: "Register"
                        enabled: networkManager.isConnected && usernameInput.text.length > 0 && passwordInput.text.length > 0
                        onClicked: {
                            if (usernameInput.text.length > 0 && passwordInput.text.length > 0)
                                networkManager.sendRegister(usernameInput.text, passwordInput.text)
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "#cdd6f4"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: parent.down ? "#45475a" : "#313244"
                            radius: 4
                        }
                    }

                    Button {
                        text: "Login"
                        enabled: networkManager.isConnected && usernameInput.text.length > 0 && passwordInput.text.length > 0
                        onClicked: {
                            if (usernameInput.text.length > 0 && passwordInput.text.length > 0)
                                networkManager.sendLogin(usernameInput.text, passwordInput.text)
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "#cdd6f4"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: parent.down ? "#45475a" : "#89b4fa"
                            radius: 4
                        }
                    }
                }

                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 8

                    Rectangle {
                        width: 12
                        height: 12
                        radius: 6
                        anchors.verticalCenter: parent.verticalCenter
                        color: networkManager.isConnected ? "#4caf50" : "#f44336"
                    }

                    Label {
                        text: networkManager.isConnected ? "Connected" : "Disconnected"
                        color: networkManager.isConnected ? "#4caf50" : "#f44336"
                        font.pixelSize: 13
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
                    color: "#1e1e2e"

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        // Sidebar header
                        Rectangle {
                            Layout.fillWidth: true
                            height: 48
                            color: "#181825"

                            Label {
                                anchors.centerIn: parent
                                text: networkManager.currentUsername
                                color: "#cdd6f4"
                                font.pixelSize: 14
                                font.bold: true
                            }
                        }

                        // New chat input
                        Row {
                            Layout.fillWidth: true
                            Layout.margins: 8
                            spacing: 6

                            TextField {
                                id: newChatInput
                                width: parent.width - goButton.width - parent.spacing
                                placeholderText: "Username..."
                                color: "#cdd6f4"
                                placeholderTextColor: "#6c7086"
                                font.pixelSize: 12
                                background: Rectangle {
                                    color: "#313244"
                                    radius: 4
                                }
                                padding: 8
                                onAccepted: {
                                    if (newChatInput.text.length > 0) {
                                        root.activePeer = newChatInput.text
                                        chatModel.clearHistory()
                                        networkManager.fetchPeerKey(newChatInput.text)
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
                                        chatModel.clearHistory()
                                        networkManager.fetchPeerKey(newChatInput.text)
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
                                height: 56
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
                                    chatModel.clearHistory()
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

                // Chat area
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 0

                    // Chat header
                    Rectangle {
                        Layout.fillWidth: true
                        height: 48
                        color: "#181825"

                        Label {
                            anchors.centerIn: parent
                            text: root.activePeer.length > 0 ? root.activePeer : "Select a conversation"
                            color: "#cdd6f4"
                            font.pixelSize: 14
                            font.bold: true
                        }
                    }

                    // Message list
                    ListView {
                        id: messageList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: chatModel
                        clip: true
                        verticalLayoutDirection: ListView.BottomToTop
                        spacing: 6

                        topMargin: 8
                        bottomMargin: 8
                        leftMargin: 12
                        rightMargin: 12

                        delegate: Item {
                            width: messageList.width - messageList.leftMargin - messageList.rightMargin
                            height: bubbleRow.implicitHeight + 4

                            Row {
                                id: bubbleRow
                                width: parent.width
                                layoutDirection: model.isOutgoing ? Qt.RightToLeft : Qt.LeftToRight

                                Rectangle {
                                    width: Math.min(bubbleLabel.implicitWidth + 20, bubbleRow.width * 0.72)
                                    height: bubbleLabel.implicitHeight + 12
                                    radius: 10
                                    color: model.isOutgoing ? "#89b4fa" : "#313244"

                                    Label {
                                        id: bubbleLabel
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.top: parent.top
                                        anchors.margins: 10
                                        text: model.content
                                        color: model.isOutgoing ? "#1e1e2e" : "#cdd6f4"
                                        font.pixelSize: 13
                                        wrapMode: Text.Wrap
                                    }
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
                                placeholderText: "Message..."
                                color: "#cdd6f4"
                                placeholderTextColor: "#6c7086"
                                background: Rectangle {
                                    color: "#313244"
                                    radius: 4
                                }
                                padding: 10
                                enabled: root.activePeer.length > 0
                                onAccepted: sendButton.clicked()
                            }

                            Button {
                                id: sendButton
                                text: "Send"
                                width: 70
                                height: parent.height
                                enabled: root.activePeer.length > 0 && msgInput.text.length > 0
                                onClicked: {
                                    var txt = msgInput.text
                                    if (txt.length === 0 || root.activePeer.length === 0) return
                                    msgInput.text = ""
                                    var ts = new Date().toISOString()
                                    chatModel.appendMessage(networkManager.currentUsername, txt, ts, true)
                                    convListModel.addOrUpdate(root.activePeer, txt, ts)
                                    networkManager.sendMessage(root.activePeer, txt)
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
