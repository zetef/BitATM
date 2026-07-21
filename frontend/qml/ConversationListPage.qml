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

    CreateGroupDialog {
        id: createGroupDialog
        isMobile: convListPage.isMobile
    }

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
                width: parent.width - goButton.width - newGroupButton.width - parent.spacing * 2
                placeholderText: "> new session..."
                color: "#c8c8c8"
                placeholderTextColor: "#505050"
                font.pixelSize: 12
                font.family: "Monospace"
                verticalAlignment: TextInput.AlignVCenter
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

            Button {
                id: newGroupButton
                text: "[+G]"
                width: 42
                height: newChatInput.height
                onClicked: createGroupDialog.open()
                contentItem: Text {
                    text: parent.text
                    color: "#c8c8c8"
                    font.pixelSize: 11
                    font.family: "Monospace"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: parent.down ? "#1a1a1a" : "transparent"
                    radius: 0
                    border.color: "#505050"
                    border.width: 1
                }
            }
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: convListModel
            clip: true

            delegate: ItemDelegate {
                id: convDelegate
                width: ListView.view.width
                height: 60
                highlighted: convListPage.activePeer === (model.is_group ? model.group_id : model.username)

                background: Rectangle {
                    color: parent.highlighted ? "#1a1a1a" : (parent.hovered ? "#111111" : "transparent")
                    border.color: parent.highlighted ? "#00ff41" : "transparent"
                    border.width: parent.highlighted ? 1 : 0
                }

                Column {
                    anchors.left: parent.left
                    anchors.right: rightControls.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 12
                    anchors.rightMargin: 6
                    spacing: 2

                    Label {
                        width: parent.width
                        text: model.is_group ? ("[G] " + model.username) : model.username
                        color: model.is_group ? "#89b4fa" : "#c8c8c8"
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

                // Badge and delete button sit side by side rather than
                // overlapping: on mobile the delete button is always visible
                // (no hover state to gate it), so it can no longer share the
                // same anchored slot as the unread badge the way it did when
                // hover alone decided which of the two was showing.
                Row {
                    id: rightControls
                    anchors.right: parent.right
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 6

                    Label {
                        id: unreadBadge
                        visible: model.unreadCount > 0
                        anchors.verticalCenter: parent.verticalCenter
                        text: "[" + (model.unreadCount > 99 ? "99+" : model.unreadCount) + "]"
                        color: "#00ff41"
                        font.pixelSize: 11
                        font.bold: true
                        font.family: "Monospace"
                    }

                    Button {
                        id: deleteBtn
                        anchors.verticalCenter: parent.verticalCenter
                        visible: convListPage.isMobile || convDelegate.hovered
                        width: 36
                        height: 24
                        text: "[x]"
                        onClicked: {
                            if (model.is_group)
                                networkManager.deleteGroup(model.group_id)
                            else
                                deleteChoiceMenu.popup()
                        }
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

                        Menu {
                            id: deleteChoiceMenu
                            MenuItem {
                                text: "Delete for me"
                                onTriggered: networkManager.deleteConversation(model.username)
                            }
                            MenuItem {
                                text: "Delete for everyone"
                                onTriggered: networkManager.deleteConversationForEveryone(model.username)
                            }
                        }
                    }
                }

                onClicked: {
                    if (model.is_group) openGroupChat(model.group_id)
                    else openChat(model.username)
                }
            }
        }

    }

    function openChat(peer) {
        if (peer.length === 0) return
        convListPage.peerSelected(peer)
        chatModel.switchConversation(peer)
        networkManager.markConversationRead(peer)
        convListModel.markRead(peer)
        networkManager.fetchPeerKey(peer)
        newChatInput.text = ""
        if (convListPage.isMobile && convListPage.stackView !== null) {
            convListPage.stackView.push(convListPage.chatPageComponent)
        }
    }

    function openGroupChat(groupId) {
        if (groupId.length === 0) return
        convListPage.peerSelected(groupId)
        chatModel.switchConversation(groupId)
        networkManager.markConversationRead(groupId)
        convListModel.markRead(groupId)
        networkManager.fetchGroupInfo(groupId)
        if (convListPage.isMobile && convListPage.stackView !== null) {
            convListPage.stackView.push(convListPage.chatPageComponent)
        }
    }
}
