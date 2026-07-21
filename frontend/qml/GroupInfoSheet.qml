import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: sheet
    color: "#0f0f0f"
    width: 300

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
        anchors.margins: 16
        spacing: 10

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Label {
                text: "GROUP INFO"
                color: "#505050"
                font.pixelSize: 10
                font.bold: true
                font.family: "Monospace"
                font.letterSpacing: 1
            }

            Label {
                text: "[ " + sheet.groupName + " ]"
                color: "#00ff41"
                font.pixelSize: 16
                font.bold: true
                font.family: "Monospace"
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Label {
                text: sheet.members.length + (sheet.members.length === 1 ? " member" : " members")
                color: "#505050"
                font.pixelSize: 11
                font.family: "Monospace"
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#1a1a1a" }

        Label {
            text: "MEMBERS"
            color: "#404040"
            font.pixelSize: 10
            font.bold: true
            font.family: "Monospace"
            font.letterSpacing: 1
        }

        ListView {
            Layout.fillWidth: true
            implicitHeight: Math.min(contentHeight, 300)
            model: sheet.members
            clip: true
            spacing: 0

            delegate: Item {
                width: ListView.view.width
                height: 60

                Rectangle {
                    anchors.fill: parent
                    color: rowHover.hovered ? "#161616" : "transparent"
                }

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 1
                    color: "#161616"
                }

                HoverHandler { id: rowHover }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 6
                    anchors.rightMargin: 6
                    anchors.topMargin: 6
                    anchors.bottomMargin: 6
                    spacing: 4

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Rectangle {
                            width: 8
                            height: 8
                            radius: 4
                            color: modelData.role === "creator" ? "#00ff41"
                                 : modelData.role === "admin"   ? "#c8c8c8" : "#404040"
                        }

                        Label {
                            Layout.fillWidth: true
                            text: modelData.username
                            color: "#c8c8c8"
                            font.pixelSize: 13
                            font.family: "Monospace"
                            elide: Text.ElideRight
                        }

                        Label {
                            text: modelData.role === "creator" ? "creator"
                                : modelData.role === "admin"   ? "admin" : ""
                            color: "#505050"
                            font.pixelSize: 10
                            font.family: "Monospace"
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Item { Layout.fillWidth: true }

                        Button {
                            visible: sheet.currentUserRole === "creator"
                                  && modelData.role === "member"
                                  && modelData.username !== networkManager.currentUsername
                            text: "[+admin]"
                            Layout.preferredWidth: 68
                            Layout.preferredHeight: 22
                            padding: 4
                            onClicked: networkManager.grantAdmin(sheet.groupId, modelData.username)
                            contentItem: Text {
                                text: parent.text; color: "#00ff41"; font.pixelSize: 10
                                font.family: "Monospace"
                                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle {
                                color: parent.down ? "#003311" : "transparent"
                                radius: 0; border.color: "#00ff41"; border.width: 1
                            }
                        }

                        Button {
                            visible: sheet.currentUserRole === "creator"
                                  && modelData.role === "admin"
                                  && modelData.username !== networkManager.currentUsername
                            text: "[-admin]"
                            Layout.preferredWidth: 68
                            Layout.preferredHeight: 22
                            padding: 4
                            onClicked: networkManager.revokeAdmin(sheet.groupId, modelData.username)
                            contentItem: Text {
                                text: parent.text; color: "#ffcc00"; font.pixelSize: 10
                                font.family: "Monospace"
                                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle {
                                color: parent.down ? "#332600" : "transparent"
                                radius: 0; border.color: "#ffcc00"; border.width: 1
                            }
                        }

                        Button {
                            visible: (sheet.currentUserRole === "creator" || sheet.currentUserRole === "admin")
                                  && modelData.role !== "creator"
                                  && modelData.username !== networkManager.currentUsername
                            text: "[kick]"
                            Layout.preferredWidth: 56
                            Layout.preferredHeight: 22
                            padding: 4
                            onClicked: networkManager.kickMember(sheet.groupId, modelData.username)
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
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#1a1a1a" }

        ColumnLayout {
            visible: sheet.currentUserRole === "creator" || sheet.currentUserRole === "admin"
            Layout.fillWidth: true
            spacing: 4

            Label {
                text: "ADD MEMBER"
                color: "#404040"
                font.pixelSize: 10
                font.bold: true
                font.family: "Monospace"
                font.letterSpacing: 1
            }

            Row {
                Layout.fillWidth: true
                width: parent.width
                spacing: 6

                TextField {
                    id: addMemberInput
                    width: parent.width - addBtn.implicitWidth - parent.spacing
                    placeholderText: "> username..."
                    color: "#c8c8c8"
                    placeholderTextColor: "#505050"
                    font.pixelSize: 12
                    font.family: "Monospace"
                    verticalAlignment: TextInput.AlignVCenter
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
        }

        Item { Layout.fillHeight: true }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#1a1a1a" }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Item { Layout.fillWidth: true }

            Button {
                text: "[leave group]"
                visible: sheet.currentUserRole !== "creator"
                Layout.preferredHeight: 28
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
                text: "[delete group]"
                visible: sheet.currentUserRole === "creator"
                Layout.preferredHeight: 28
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
                text: "[close]"
                Layout.preferredHeight: 28
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
}
