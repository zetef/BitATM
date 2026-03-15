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

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Hello from Qt"
            font.pixelSize: 18
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
