import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: convListPage
    color: "#0f0f0f"

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
            color: "#060606"

            Label {
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                text: networkManager.currentUsername + "@bitatm"
                color: "#00ff41"
                font.pixelSize: 14
                font.bold: true
                font.family: "Monospace"
            }

            Button {
                anchors.right: parent.right
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                text: "[logout]"
                font.pixelSize: 11
                onClicked: networkManager.logout()
                contentItem: Text {
                    text: parent.text
                    color: "#ff3333"
                    font.pixelSize: 11
                    font.family: "Monospace"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: parent.down ? "#1a0000" : "transparent"
                    radius: 0
                    border.color: "#ff3333"
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
                placeholderText: "> new session..."
                color: "#c8c8c8"
                placeholderTextColor: "#505050"
                font.pixelSize: 12
                font.family: "Monospace"
                background: Rectangle { color: "#1a1a1a"; radius: 0 }
                padding: 8
                onAccepted: openChat(newChatInput.text)
            }

            Button {
                id: goButton
                text: ">>"
                width: 36
                height: newChatInput.height
                onClicked: openChat(newChatInput.text)
                contentItem: Text {
                    text: parent.text
                    color: "#0a0a0a"
                    font.pixelSize: 12
                    font.family: "Monospace"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: parent.down ? "#00cc33" : "#00ff41"
                    radius: 0
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
                    color: parent.highlighted ? "#1a1a1a" : (parent.hovered ? "#111111" : "transparent")
                    border.color: parent.highlighted ? "#00ff41" : "transparent"
                    border.width: parent.highlighted ? 1 : 0
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
                        color: "#c8c8c8"
                        font.pixelSize: 13
                        font.bold: true
                        font.family: "Monospace"
                        elide: Text.ElideRight
                    }

                    Label {
                        width: parent.width
                        text: model.lastMessage
                        color: "#505050"
                        font.pixelSize: 11
                        font.family: "Monospace"
                        elide: Text.ElideRight
                    }
                }

                onClicked: openChat(model.username)
            }
        }

        Button {
            Layout.fillWidth: true
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            Layout.bottomMargin: 8
            height: 32
            text: "+ New Group"
            onClicked: createGroupDialog.visible = true
            contentItem: Text {
                text: parent.text; color: "#89b4fa"; font.pixelSize: 12
                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                color: parent.down ? "#313244" : "transparent"
                radius: 4; border.color: "#313244"; border.width: 1
            }
        }
    }

    CreateGroupDialog {
        id: createGroupDialog
        anchors.centerIn: parent
        visible: false
        z: 10
        onClosed: createGroupDialog.visible = false
    }

    function openChat(peer) {
        if (peer.length === 0) return
        convListPage.peerSelected(peer)
        chatModel.switchConversation(peer)
        var isGroup = /^\d+$/.test(peer)
        if (!isGroup) {
            networkManager.markConversationRead(peer)
            networkManager.fetchPeerKey(peer)
        } else {
            networkManager.fetchGroupInfo(peer)
        }
        newChatInput.text = ""
        if (convListPage.isMobile && convListPage.stackView !== null) {
            convListPage.stackView.push(convListPage.chatPageComponent)
        }
    }
}
