import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: chatPage
    color: "#1e1e2e"

    property string activePeer: ""
    property bool isMobile: false
    property var stackView: null

    Column {
        visible: chatPage.activePeer.length === 0
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

    ColumnLayout {
        visible: chatPage.activePeer.length > 0
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            height: 48
            color: "#181825"

            Button {
                id: backButton
                visible: chatPage.isMobile
                anchors.left: parent.left
                anchors.leftMargin: 4
                anchors.verticalCenter: parent.verticalCenter
                text: "<"
                width: 36
                height: 36
                onClicked: {
                    if (chatPage.stackView !== null)
                        chatPage.stackView.pop()
                }
                contentItem: Text {
                    text: parent.text
                    color: "#cdd6f4"
                    font.pixelSize: 18
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: parent.down ? "#313244" : "transparent"
                    radius: 4
                }
            }

            Label {
                anchors.centerIn: parent
                text: chatPage.activePeer
                color: "#cdd6f4"
                font.pixelSize: 14
                font.bold: true
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

                    Label {
                        id: statusLabel
                        visible: model.isOutgoing
                        width: parent.width
                        text: model.status === "seen"      ? "✓✓"
                            : model.status === "delivered" ? "✓✓"
                            :                                "✓"
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
                    placeholderText: "Message " + chatPage.activePeer + "..."
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
                        chatModel.appendAndCache(chatPage.activePeer,
                                                networkManager.currentUsername,
                                                txt, ts, true)
                        convListModel.addOrUpdate(chatPage.activePeer, txt, ts)
                        networkManager.sendMessage(chatPage.activePeer, txt, ts)
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
