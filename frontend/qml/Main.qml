import QtQuick
import QtQuick.Controls

ApplicationWindow {
    visible: true
    width: 400
    height: 300
    title: "BitATM"

    Column {
        anchors.centerIn: parent
        spacing: 12

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 8

            TextField {
                id: messageInput
                placeholderText: "Type a message..."
                width: 240
                onAccepted: sendButton.clicked()
            }

            Button {
                id: sendButton
                text: "Send"
                enabled: networkManager.isConnected
                onClicked: {
                    if (messageInput.text.length > 0) {
                        networkManager.sendText(messageInput.text)
                        messageInput.text = ""
                    }
                }
            }
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: networkManager.lastMessage ? "Server: " + networkManager.lastMessage : "No response yet"
            font.pixelSize: 14
            color: networkManager.lastMessage
                ? (networkManager.hasError ? "#f44336" : "#4caf50")
                : "#999"
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 8

            Rectangle {
                width: 12; height: 12; radius: 6
                anchors.verticalCenter: parent.verticalCenter
                color: networkManager.isConnected ? "#4caf50" : "#f44336"
            }

            Label {
                text: networkManager.isConnected ? "Connected" : "Disconnected"
                color: networkManager.isConnected ? "#4caf50" : "#f44336"
            }
        }
    }
}
