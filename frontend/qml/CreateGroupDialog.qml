import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: dialog
    color: "#252535"
    radius: 8
    width: 320
    height: col.implicitHeight + 32

    signal closed()

    ColumnLayout {
        id: col
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 16
        spacing: 10

        Label {
            text: "New Group"
            color: "#cdd6f4"
            font.pixelSize: 16
            font.bold: true
        }

        TextField {
            id: groupNameInput
            Layout.fillWidth: true
            placeholderText: "Group name"
            color: "#cdd6f4"
            placeholderTextColor: "#585b70"
            background: Rectangle { color: "#313244"; radius: 4 }
            padding: 8
        }

        TextField {
            id: membersInput
            Layout.fillWidth: true
            placeholderText: "Members (comma-separated usernames)"
            color: "#cdd6f4"
            placeholderTextColor: "#585b70"
            background: Rectangle { color: "#313244"; radius: 4 }
            padding: 8
        }

        Label {
            visible: membersInput.text.length > 0
            text: {
                var parts = membersInput.text.split(",").filter(function(s) { return s.trim().length > 0 })
                return "You + " + parts.length + " member(s)"
            }
            color: "#6c7086"
            font.pixelSize: 11
        }

        Row {
            spacing: 8

            Button {
                text: "Cancel"
                onClicked: dialog.closed()
                contentItem: Text {
                    text: parent.text; color: "#cdd6f4"; font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle { color: parent.down ? "#45475a" : "#313244"; radius: 4 }
            }

            Button {
                text: "Create"
                enabled: groupNameInput.text.trim().length > 0 && membersInput.text.trim().length > 0
                onClicked: {
                    var rawParts = membersInput.text.split(",")
                    var memberList = []
                    for (var i = 0; i < rawParts.length; ++i) {
                        var s = rawParts[i].trim()
                        if (s.length > 0) memberList.push(s)
                    }
                    networkManager.createGroup(groupNameInput.text.trim(), memberList)
                    groupNameInput.text = ""
                    membersInput.text = ""
                    dialog.closed()
                }
                contentItem: Text {
                    text: parent.text; color: "#1e1e2e"; font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: parent.enabled ? (parent.down ? "#74a0e8" : "#89b4fa") : "#45475a"
                    radius: 4
                }
            }
        }
    }
}
