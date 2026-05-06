import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: chatPage
    color: "#0a0a0a"

    property string activePeer: ""
    property bool isMobile: false
    property var stackView: null
    property bool _isGroup: /^\d+$/.test(chatPage.activePeer)

    GroupInfoSheet {
        id: groupInfoSheet
        visible: false
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        z: 10
        groupId: chatPage.activePeer
        onClosed: groupInfoSheet.visible = false

        Connections {
            target: networkManager
            function onGroupInfoReceived(gid, name, members) {
                if (gid !== chatPage.activePeer) return
                groupInfoSheet.groupName = name
                groupInfoSheet.members = members
                for (var i = 0; i < members.length; ++i) {
                    if (members[i].username === networkManager.currentUsername) {
                        groupInfoSheet.currentUserRole = members[i].role
                        break
                    }
                }
            }
        }
    }

    Column {
        visible: chatPage.activePeer.length === 0
        anchors.centerIn: parent
        spacing: 12

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "BitATM - secure channel"
            color: "#00ff41"
            font.pixelSize: 22
            font.bold: true
            font.family: "Monospace"
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "no active session. select target."
            color: "#505050"
            font.pixelSize: 14
            font.family: "Monospace"
        }
    }

    ColumnLayout {
        visible: chatPage.activePeer.length > 0
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            height: 48
            color: "#0f0f0f"

            Button {
                id: backButton
                visible: chatPage.isMobile
                anchors.left: parent.left
                anchors.leftMargin: 4
                anchors.verticalCenter: parent.verticalCenter
                text: "<<"
                width: 36
                height: 36
                onClicked: {
                    if (chatPage.stackView !== null)
                        chatPage.stackView.pop()
                }
                contentItem: Text {
                    text: parent.text
                    color: "#c8c8c8"
                    font.pixelSize: 18
                    font.family: "Monospace"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: parent.down ? "#1a1a1a" : "transparent"
                    radius: 0
                }
            }

            Label {
                anchors.centerIn: parent
                text: "[ " + chatPage.activePeer + " ]"
                color: "#c8c8c8"
                font.pixelSize: 14
                font.bold: true
                font.family: "Monospace"
            }

            Button {
                visible: chatPage._isGroup
                anchors.right: parent.right
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                text: "[info]"
                width: 52
                height: 28
                onClicked: {
                    networkManager.fetchGroupInfo(chatPage.activePeer)
                    groupInfoSheet.visible = true
                }
                contentItem: Text {
                    text: parent.text; color: "#00ff41"; font.pixelSize: 11
                    font.family: "Monospace"
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle { color: parent.down ? "#1a1a1a" : "transparent"; radius: 0 }
            }
        }

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
                            radius: 0
                            color: model.isOutgoing ? "#00ff41" : "#1a1a1a"
                            anchors.right: model.isOutgoing ? parent.right : undefined
                            anchors.left:  model.isOutgoing ? undefined : parent.left

                            Label {
                                id: bubbleLabel
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.margins: 12
                                text: model.content
                                color: model.isOutgoing ? "#0a0a0a" : "#c8c8c8"
                                font.pixelSize: 13
                                font.family: "Monospace"
                                wrapMode: Text.Wrap
                            }
                        }
                    }

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
                        color: "#404040"
                        font.pixelSize: 10
                        font.family: "Monospace"
                        horizontalAlignment: model.isOutgoing ? Text.AlignRight : Text.AlignLeft
                    }

                    Label {
                        id: statusLabel
                        visible: model.isOutgoing
                        width: parent.width
                        text: model.status === "seen"      ? "✓✓"
                            : model.status === "delivered" ? "✓✓"
                            :                                "✓"
                        color: model.status === "seen" ? "#00ff41" : "#404040"
                        font.pixelSize: 10
                        font.family: "Monospace"
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

        Rectangle {
            Layout.fillWidth: true
            height: 56
            color: "#0f0f0f"

            Row {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                TextField {
                    id: msgInput
                    width: parent.width - sendButton.width - parent.spacing
                    height: parent.height
                    placeholderText: "> msg " + chatPage.activePeer + "..."
                    color: "#c8c8c8"
                    placeholderTextColor: "#505050"
                    font.family: "Monospace"
                    background: Rectangle { color: "#1a1a1a"; radius: 0 }
                    padding: 10
                    onAccepted: sendButton.clicked()
                }

                Button {
                    id: sendButton
                    text: ">> send"
                    width: 80
                    height: parent.height
                    enabled: msgInput.text.length > 0
                    onClicked: {
                        var txt = msgInput.text
                        if (txt.length === 0) return
                        msgInput.text = ""
                        var ts = new Date().toISOString()
                        var peer = chatPage.activePeer
                        chatModel.appendAndCache(peer, networkManager.currentUsername, txt, ts, true)
                        if (chatPage._isGroup) {
                            convListModel.addOrUpdateGroup(peer, "", txt, ts)
                            networkManager.sendGroupMessage(peer, txt, ts)
                        } else {
                            convListModel.addOrUpdate(peer, txt, ts)
                            networkManager.sendMessage(peer, txt, ts)
                        }
                    }
                    contentItem: Text {
                        text: parent.text
                        color: "#0a0a0a"
                        font.pixelSize: 13
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
        }
    }
}
