import QtQuick
import QtQuick.Controls

ApplicationWindow {
    visible: true
    width: 400
    height: 380
    title: "BitATM"

    property string statusMessage: ""

    Connections {
        target: networkManager
        function onLastMessageChanged() {
            statusMessage = networkManager.lastMessage
            messageTimer.restart()
        }
    }

    Timer {
        id: messageTimer
        interval: 4000
        onTriggered: statusMessage = ""
    }

    Column {
        anchors.centerIn: parent
        spacing: 12
        width: 300

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: networkManager.currentUsername.length > 0
            text: "Logged in as: " + networkManager.currentUsername
            font.pixelSize: 13
            font.bold: true
            color: "#4caf50"
        }

        TextField {
            id: usernameInput
            placeholderText: "Username"
            width: parent.width
        }

        TextField {
            id: passwordInput
            placeholderText: "Password"
            echoMode: TextInput.Password
            width: parent.width
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 8

            Button {
                text: "Register"
                enabled: networkManager.isConnected
                onClicked: {
                    if (usernameInput.text.length > 0 && passwordInput.text.length > 0)
                        networkManager.sendRegister(usernameInput.text, passwordInput.text)
                }
            }

            Button {
                text: "Login"
                enabled: networkManager.isConnected
                onClicked: {
                    if (usernameInput.text.length > 0 && passwordInput.text.length > 0)
                        networkManager.sendLogin(usernameInput.text, passwordInput.text)
                }
            }
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: statusMessage.length > 0
            text: statusMessage
            font.pixelSize: 14
            color: networkManager.hasError ? "#f44336" : "#4caf50"
            wrapMode: Text.Wrap
            width: parent.width
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
