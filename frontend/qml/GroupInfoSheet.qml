import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: sheet
    color: "#252535"
    radius: 8
    width: 260

    property string groupId: ""
    property string groupName: ""
    property var members: []
    property string currentUserRole: "member"

    signal closed()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        Label {
            text: sheet.groupName
            color: "#cdd6f4"
            font.pixelSize: 15
            font.bold: true
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
                            : modelData.role === "admin"   ? "[A]" : ""
                        color: modelData.role === "creator" ? "#f9e2af"
                             : modelData.role === "admin"   ? "#a6e3a1" : "#6c7086"
                        font.pixelSize: 10
                        font.bold: true
                    }

                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        text: modelData.username
                        color: "#cdd6f4"
                        font.pixelSize: 13
                    }

                    Button {
                        anchors.verticalCenter: parent.verticalCenter
                        visible: (sheet.currentUserRole === "creator" || sheet.currentUserRole === "admin")
                              && modelData.role !== "creator"
                              && modelData.username !== networkManager.currentUsername
                        text: "Kick"
                        height: 24
                        onClicked: {
                            networkManager.kickMember(sheet.groupId, modelData.username)
                            sheet.closed()
                        }
                        contentItem: Text {
                            text: parent.text; color: "#f38ba8"; font.pixelSize: 10
                            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: parent.down ? "#3b1e1e" : "transparent"
                            radius: 3; border.color: "#f38ba8"; border.width: 1
                        }
                    }
                }
            }
        }

        Row {
            visible: sheet.currentUserRole === "creator" || sheet.currentUserRole === "admin"
            spacing: 6
            Layout.fillWidth: true

            TextField {
                id: addMemberInput
                width: parent.width - addBtn.implicitWidth - parent.spacing
                placeholderText: "Add member..."
                color: "#cdd6f4"
                placeholderTextColor: "#585b70"
                font.pixelSize: 12
                background: Rectangle { color: "#313244"; radius: 4 }
                padding: 6
            }

            Button {
                id: addBtn
                text: "Add"
                onClicked: {
                    var u = addMemberInput.text.trim()
                    if (u.length > 0) {
                        networkManager.addGroupMember(sheet.groupId, u)
                        addMemberInput.text = ""
                    }
                }
                contentItem: Text {
                    text: parent.text; color: "#cdd6f4"; font.pixelSize: 11
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle { color: parent.down ? "#45475a" : "#89b4fa"; radius: 3 }
            }
        }

        Button {
            Layout.alignment: Qt.AlignRight
            text: "Leave Group"
            visible: sheet.currentUserRole !== "creator"
            onClicked: {
                networkManager.leaveGroup(sheet.groupId)
                sheet.closed()
            }
            contentItem: Text {
                text: parent.text; color: "#f38ba8"; font.pixelSize: 11
                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                color: parent.down ? "#3b1e1e" : "transparent"
                radius: 3; border.color: "#f38ba8"; border.width: 1
            }
        }

        Button {
            Layout.alignment: Qt.AlignRight
            text: "Close"
            onClicked: sheet.closed()
            contentItem: Text {
                text: parent.text; color: "#cdd6f4"; font.pixelSize: 12
                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle { color: parent.down ? "#45475a" : "#313244"; radius: 4 }
        }
    }
}
