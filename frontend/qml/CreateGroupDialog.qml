import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

Item {
    id: root
    property bool isMobile: false
    signal closed()

    function open() {
        if (root.isMobile) mobileDrawer.open()
        else desktopPopup.open()
    }

    // --- Desktop: centered Popup over full window ---
    Popup {
        id: desktopPopup
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        padding: 20
        width: 360

        Overlay.modal: Rectangle { color: "#0a0a0a"; opacity: 0.75 }

        background: Rectangle {
            color: "#0f0f0f"
            border.color: "#404040"
            border.width: 1
            radius: 0
        }

        contentItem: ColumnLayout {
            spacing: 10

            Label {
                text: "> new group"
                color: "#00ff41"
                font.pixelSize: 15
                font.bold: true
                font.family: "Monospace"
            }

            TextField {
                id: desktopGroupName
                Layout.fillWidth: true
                placeholderText: "> group name"
                color: "#c8c8c8"
                placeholderTextColor: "#505050"
                font.family: "Monospace"
                background: Rectangle { color: "#1a1a1a"; radius: 0 }
                padding: 8
            }

            TextField {
                id: desktopMembers
                Layout.fillWidth: true
                placeholderText: "> members (comma-separated)"
                color: "#c8c8c8"
                placeholderTextColor: "#505050"
                font.family: "Monospace"
                background: Rectangle { color: "#1a1a1a"; radius: 0 }
                padding: 8
            }

            Label {
                visible: desktopMembers.text.length > 0
                text: {
                    var parts = desktopMembers.text.split(",").filter(function(s) { return s.trim().length > 0 })
                    return "you + " + parts.length + " member(s)"
                }
                color: "#505050"
                font.pixelSize: 11
                font.family: "Monospace"
            }

            Row {
                spacing: 8

                Button {
                    text: "[cancel]"
                    onClicked: desktopPopup.close()
                    contentItem: Text {
                        text: parent.text; color: "#c8c8c8"; font.pixelSize: 12
                        font.family: "Monospace"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.down ? "#222222" : "#1a1a1a"
                        radius: 0; border.color: "#505050"; border.width: 1
                    }
                }

                Button {
                    text: "[+ create]"
                    enabled: desktopGroupName.text.trim().length > 0 && desktopMembers.text.trim().length > 0
                    onClicked: {
                        var rawParts = desktopMembers.text.split(",")
                        var memberList = []
                        for (var i = 0; i < rawParts.length; ++i) {
                            var s = rawParts[i].trim()
                            if (s.length > 0) memberList.push(s)
                        }
                        networkManager.createGroup(desktopGroupName.text.trim(), memberList)
                        desktopGroupName.text = ""
                        desktopMembers.text = ""
                        desktopPopup.close()
                    }
                    contentItem: Text {
                        text: parent.text; color: "#0a0a0a"; font.pixelSize: 12
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

        onClosed: root.closed()
    }

    // --- Mobile: bottom sheet Popup ---
    // Plain Popup, not Drawer: Drawer positions itself via its own internal
    // edge-docking transform, which fights any manual y binding. A Popup
    // gives full manual control, needed to track the keyboard directly.
    //
    // windowSoftInputMode="adjustResize" (AndroidManifest.xml) does not
    // reliably make Qt's own render surface shrink on Android even though
    // the OS resizes the activity window - confirmed on-device. So instead
    // of relying on the viewport shrinking, this tracks the live keyboard
    // geometry directly via Qt.inputMethod.keyboardRectangle (the documented
    // API for this, independent of whether the window itself resizes) and
    // pins the sheet's bottom edge just above the keyboard.
    Popup {
        id: mobileDrawer
        parent: Overlay.overlay
        modal: true
        padding: 0

        // Reactive bindings alone are not reliable here: Qt.inputMethod.visible,
        // keyboardRectangle, and mobileForm.implicitHeight were confirmed
        // on-device to update in separate, inconsistent steps within the same
        // burst of change signals (e.g. visible flips true a moment before
        // keyboardRectangle has real geometry), so any single reactive
        // expression combining them can transiently see a mismatched
        // combination. Instead: debounce with a short settle timer, then
        // compute the final geometry once, atomically, in JS.
        x: 0
        y: 0
        width: Overlay.overlay ? Overlay.overlay.width : 400
        height: 218

        // Qt.inputMethod.keyboardRectangle is reported in PHYSICAL pixels on
        // Android, while every other QML geometry value (Overlay.overlay,
        // Popup.y/height) is in logical/DPI-scaled pixels - confirmed via
        // on-device logging (keyboardRectangle.y was larger than the entire
        // logical viewport height). Must divide by devicePixelRatio to bring
        // it into the same coordinate space as everything else here.
        readonly property real dpr: Screen.devicePixelRatio > 0 ? Screen.devicePixelRatio : 1

        function recalcGeometry() {
            var viewportHeight = Overlay.overlay ? Overlay.overlay.height : 640
            var kbVisible = Qt.inputMethod.visible
            var kbRect = Qt.inputMethod.keyboardRectangle
            var availableBottom = (kbVisible && kbRect.height > 0) ? (kbRect.y / dpr) : viewportHeight
            var h = Math.max(100, Math.min(mobileForm.implicitHeight + 48, availableBottom - 20))
            height = h
            y = Math.max(0, availableBottom - h)
        }

        Timer {
            id: settleTimer
            interval: 80
            onTriggered: mobileDrawer.recalcGeometry()
        }

        Connections {
            target: Qt.inputMethod
            function onVisibleChanged() { settleTimer.restart() }
            function onKeyboardRectangleChanged() { settleTimer.restart() }
        }

        onOpened: recalcGeometry()

        Overlay.modal: Rectangle { color: "#0a0a0a"; opacity: 0.75 }

        background: Rectangle {
            color: "#0f0f0f"
            Rectangle { width: parent.width; height: 1; color: "#404040" }
        }

        ScrollView {
            anchors.fill: parent
            anchors.margins: 20
            clip: true

        ColumnLayout {
            id: mobileForm
            width: parent.width
            spacing: 10

            Label {
                text: "> new group"
                color: "#00ff41"
                font.pixelSize: 15
                font.bold: true
                font.family: "Monospace"
            }

            TextField {
                id: mobileGroupName
                Layout.fillWidth: true
                placeholderText: "> group name"
                color: "#c8c8c8"
                placeholderTextColor: "#505050"
                font.family: "Monospace"
                background: Rectangle { color: "#1a1a1a"; radius: 0 }
                padding: 8
            }

            TextField {
                id: mobileMembers
                Layout.fillWidth: true
                placeholderText: "> members (comma-separated)"
                color: "#c8c8c8"
                placeholderTextColor: "#505050"
                font.family: "Monospace"
                background: Rectangle { color: "#1a1a1a"; radius: 0 }
                padding: 8
            }

            Label {
                visible: mobileMembers.text.length > 0
                text: {
                    var parts = mobileMembers.text.split(",").filter(function(s) { return s.trim().length > 0 })
                    return "you + " + parts.length + " member(s)"
                }
                color: "#505050"
                font.pixelSize: 11
                font.family: "Monospace"
            }

            Row {
                spacing: 8

                Button {
                    text: "[cancel]"
                    onClicked: mobileDrawer.close()
                    contentItem: Text {
                        text: parent.text; color: "#c8c8c8"; font.pixelSize: 12
                        font.family: "Monospace"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.down ? "#222222" : "#1a1a1a"
                        radius: 0; border.color: "#505050"; border.width: 1
                    }
                }

                Button {
                    text: "[+ create]"
                    enabled: mobileGroupName.text.trim().length > 0 && mobileMembers.text.trim().length > 0
                    onClicked: {
                        var rawParts = mobileMembers.text.split(",")
                        var memberList = []
                        for (var i = 0; i < rawParts.length; ++i) {
                            var s = rawParts[i].trim()
                            if (s.length > 0) memberList.push(s)
                        }
                        networkManager.createGroup(mobileGroupName.text.trim(), memberList)
                        mobileGroupName.text = ""
                        mobileMembers.text = ""
                        mobileDrawer.close()
                    }
                    contentItem: Text {
                        text: parent.text; color: "#0a0a0a"; font.pixelSize: 12
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

        onClosed: root.closed()
    }
}
