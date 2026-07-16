import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Per-member receipt breakdown for one of your group messages (receipts v2).
Rectangle {
    id: infoSheet
    color: "#0f0f0f"
    width: 260
    visible: false

    property var receipts: []   // [{member, delivered, seen}]

    signal closed()

    function openFor(receiptList) {
        receipts = receiptList
        visible = true
    }

    function section(title, filter) {
        return { title: title, entries: receipts.filter(filter) }
    }

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

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: "[ message info ]"
                color: "#00ff41"
                font.pixelSize: 14
                font.bold: true
                font.family: "Monospace"
                Layout.fillWidth: true
            }
            Label {
                text: "x"
                color: "#c8c8c8"
                font.family: "Monospace"
                font.pixelSize: 14
                MouseArea {
                    anchors.fill: parent
                    anchors.margins: -8
                    cursorShape: Qt.PointingHandCursor
                    onClicked: { infoSheet.visible = false; infoSheet.closed() }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#1a1a1a"
        }

        Repeater {
            model: [
                infoSheet.section("Seen by", function(r) { return r.seen }),
                infoSheet.section("Delivered to", function(r) { return r.delivered && !r.seen }),
                infoSheet.section("Pending", function(r) { return !r.delivered })
            ]
            delegate: ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Label {
                    text: modelData.title
                    color: "#404040"
                    font.family: "Monospace"
                    font.pixelSize: 11
                }
                Repeater {
                    model: modelData.entries
                    delegate: Label {
                        text: modelData.member
                        color: "#c8c8c8"
                        font.family: "Monospace"
                        font.pixelSize: 12
                    }
                }
                Label {
                    visible: modelData.entries.length === 0
                    text: "-"
                    color: "#2a2a2a"
                    font.family: "Monospace"
                    font.pixelSize: 12
                }
            }
        }

        Label {
            visible: infoSheet.receipts.length === 0
            text: "No receipt data"
            color: "#404040"
            font.family: "Monospace"
            font.pixelSize: 12
        }

        Item { Layout.fillHeight: true }
    }
}
