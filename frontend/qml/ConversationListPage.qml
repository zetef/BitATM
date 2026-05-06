import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: convListPage
    color: "#181825"

    property string activePeer: ""
    property bool isMobile: false
    property var stackView: null
    property var chatPageComponent: null

    signal peerSelected(string peer)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

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
                onAccepted: openChat(newChatInput.text)
            }

            Button {
                id: goButton
                text: "Go"
                width: 36
                height: newChatInput.height
                onClicked: openChat(newChatInput.text)
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

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: convListModel
            clip: true

            delegate: ItemDelegate {
                width: ListView.view.width
                height: 60
                highlighted: convListPage.activePeer === model.username

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

                onClicked: openChat(model.username)
            }
        }
    }

    function openChat(peer) {
        if (peer.length === 0) return
        convListPage.peerSelected(peer)
        chatModel.switchConversation(peer)
        networkManager.markConversationRead(peer)
        networkManager.fetchPeerKey(peer)
        newChatInput.text = ""
        if (convListPage.isMobile && convListPage.stackView !== null) {
            convListPage.stackView.push(convListPage.chatPageComponent)
        }
    }
}
