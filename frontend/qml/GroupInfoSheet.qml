import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: sheet
    color: "#0f0f0f"
    width: 260

    property string groupId: ""
    property string groupName: ""
    property var members: []
    property string currentUserRole: "member"

    signal closed()

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 1
        color: "#1a1a1a"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        Label {
            text: "[ " + sheet.groupName + " ]"
            color: "#00ff41"
            font.pixelSize: 14
            font.bold: true
            font.family: "Monospace"
            elide: Text.ElideRight
            Layout.fillWidth: true
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#1a1a1a"
        }

        ListView {
            Layout.fillWidth: true
            implicitHeight: Math.min(contentHeight, 280)
            model: sheet.members
            clip: true

            delegate: Item {
                width: ListView.view.width
                height: 40

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: 4
                    spacing: 6

                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData.role === "creator" ? "[C]"
                            : modelData.role === "admin"   ? "[A]" : "   "
                        color: modelData.role === "creator" ? "#00ff41"
                             : modelData.role === "admin"   ? "#c8c8c8" : "#505050"
                        font.pixelSize: 10
                        font.bold: true
                        font.family: "Monospace"
                    }

                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData.username
                        color: "#c8c8c8"
                        font.pixelSize: 13
                        font.family: "Monospace"
                    }

                    Button {
                        anchors.verticalCenter: parent.verticalCenter
                        visible: (sheet.currentUserRole === "creator" || sheet.currentUserRole === "admin")
                              && modelData.role !== "creator"
                              && modelData.username !== networkManager.currentUsername
                        text: "[kick]"
                        height: 24
                        onClicked: {
                            networkManager.kickMember(sheet.groupId, modelData.username)
                            sheet.closed()
                        }
                        contentItem: Text {
                            text: parent.text; color: "#ff3333"; font.pixelSize: 10
                            font.family: "Monospace"
                            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: parent.down ? "#1a0000" : "transparent"
                            radius: 0; border.color: "#ff3333"; border.width: 1
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#1a1a1a"
        }

        Row {
            visible: sheet.currentUserRole === "creator" || sheet.currentUserRole === "admin"
            spacing: 6
            Layout.fillWidth: true

            TextField {
                id: addMemberInput
                width: parent.width - addBtn.implicitWidth - parent.spacing
                placeholderText: "> add member..."
                color: "#c8c8c8"
                placeholderTextColor: "#505050"
                font.pixelSize: 12
                font.family: "Monospace"
                background: Rectangle { color: "#1a1a1a"; radius: 0 }
                padding: 6
            }

            Button {
                id: addBtn
                text: ">>"
                onClicked: {
                    var u = addMemberInput.text.trim()
                    if (u.length > 0) {
                        networkManager.addGroupMember(sheet.groupId, u)
                        addMemberInput.text = ""
                    }
                }
                contentItem: Text {
                    text: parent.text; color: "#0a0a0a"; font.pixelSize: 11
                    font.family: "Monospace"
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle { color: parent.down ? "#00cc33" : "#00ff41"; radius: 0 }
            }
        }

        Button {
            Layout.alignment: Qt.AlignRight
            text: "[leave group]"
            visible: sheet.currentUserRole !== "creator"
            onClicked: {
                networkManager.leaveGroup(sheet.groupId)
                sheet.closed()
            }
            contentItem: Text {
                text: parent.text; color: "#ff3333"; font.pixelSize: 11
                font.family: "Monospace"
                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                color: parent.down ? "#1a0000" : "transparent"
                radius: 0; border.color: "#ff3333"; border.width: 1
            }
        }

        Button {
            Layout.alignment: Qt.AlignRight
            text: "[delete group]"
            visible: sheet.currentUserRole === "creator"
            onClicked: {
                networkManager.deleteGroup(sheet.groupId)
                sheet.closed()
            }
            contentItem: Text {
                text: parent.text; color: "#ff3333"; font.pixelSize: 11
                font.family: "Monospace"
                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                color: parent.down ? "#1a0000" : "transparent"
                radius: 0; border.color: "#ff3333"; border.width: 1
            }
        }

        Button {
            Layout.alignment: Qt.AlignRight
            text: "[close]"
            onClicked: sheet.closed()
            contentItem: Text {
                text: parent.text; color: "#c8c8c8"; font.pixelSize: 12
                font.family: "Monospace"
                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                color: parent.down ? "#1a1a1a" : "transparent"
                radius: 0; border.color: "#404040"; border.width: 1
            }
        }
    }
}
