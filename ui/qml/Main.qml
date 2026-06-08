// MilCAM main window — touch-first, panel-PC-friendly.
// Geometry note: minimum target is the VEC-VE 12" 1024×768 panel.

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.VirtualKeyboard

ApplicationWindow {
    id: root
    visible: true
    width: 1280
    height: 800
    title: qsTr("MilCAM — CAM for CodeSys CNC")

    // Touch-first sizing: 56px is comfortably tappable on a 12" panel.
    readonly property int touchUnit: 56

    header: ToolBar {
        height: touchUnit + 8
        RowLayout {
            anchors.fill: parent
            anchors.margins: 4
            spacing: 8

            ToolButton { text: qsTr("Import");      Layout.preferredHeight: root.touchUnit; font.pixelSize: 18; onClicked: importDialog.open() }
            ToolButton { text: qsTr("Job");          Layout.preferredHeight: root.touchUnit; font.pixelSize: 18 }
            ToolButton { text: qsTr("Operations"); Layout.preferredHeight: root.touchUnit; font.pixelSize: 18 }
            ToolButton { text: qsTr("Toolpath");    Layout.preferredHeight: root.touchUnit; font.pixelSize: 18 }
            ToolButton { text: qsTr("Post + G-code"); Layout.preferredHeight: root.touchUnit; font.pixelSize: 18 }
            Item { Layout.fillWidth: true }
            ToolButton {
                text: qsTr("→ Send to CodeSys")
                Layout.preferredHeight: root.touchUnit
                font.pixelSize: 18
                // Calls CodesysBridge.dropGCode + notifyJobReady (see WORKPLAN Faz 3).
            }
            ToolButton {
                text: qsTr("↔ HMI")
                Layout.preferredHeight: root.touchUnit
                font.pixelSize: 18
                // Switcher overlay: raises CodeSys TargetVisu window via wmctrl.
            }
        }
    }

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        // Left: job/operation tree
        Pane {
            SplitView.preferredWidth: 280
            SplitView.minimumWidth: 200
            ColumnLayout {
                anchors.fill: parent
                Label { text: qsTr("Job Tree"); font.bold: true; font.pixelSize: 16 }
                Rectangle {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    color: "#1e1e1e"; radius: 4
                    Label {
                        anchors.centerIn: parent
                        text: qsTr("(empty)\nImport a STEP/DXF to start.")
                        color: "#888"
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
        }

        // Center: 3D viewport + bottom G-code preview
        SplitView {
            SplitView.fillWidth: true
            orientation: Qt.Vertical

            Rectangle {
                SplitView.fillHeight: true
                color: "#101418"
                Label {
                    anchors.centerIn: parent
                    text: qsTr("3D viewport\n(OCCT V3d_Viewer — wired in Faz 2)")
                    color: "#666"
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            Pane {
                SplitView.preferredHeight: 180
                SplitView.minimumHeight: 120
                ColumnLayout {
                    anchors.fill: parent
                    Label { text: qsTr("G-code preview"); font.bold: true; font.pixelSize: 14 }
                    ScrollView {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        TextArea {
                            id: gcodePreview
                            readOnly: true
                            font.family: "monospace"
                            font.pixelSize: 12
                            placeholderText: qsTr("Generated G-code appears here after Post.")
                        }
                    }
                }
            }
        }

        // Right: properties of selected operation
        Pane {
            SplitView.preferredWidth: 320
            SplitView.minimumWidth: 240
            ColumnLayout {
                anchors.fill: parent
                Label { text: qsTr("Operation Properties"); font.bold: true; font.pixelSize: 16 }
                Rectangle {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    color: "#1e1e1e"; radius: 4
                    Label {
                        anchors.centerIn: parent
                        text: qsTr("Select an operation to edit\nfeeds, speeds, depth, stepover…")
                        color: "#888"
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
        }
    }

    footer: ToolBar {
        height: 36
        RowLayout {
            anchors.fill: parent
            anchors.margins: 4
            Label {
                id: statusLabel
                text: qsTr("Ready — Phase 0 scaffold, see .ai/WORKPLAN.md")
                Layout.fillWidth: true
            }
            Label {
                text: qsTr("PLC: disconnected")
                color: "#cc4444"
            }
        }
    }

    // Qt Virtual Keyboard — appears automatically when a text input gains focus.
    // No physical keyboard required on the VEC-VE panel.
    InputPanel {
        id: inputPanel
        z: 99
        x: 0
        y: root.height
        width: root.width
        states: State {
            name: "visible"
            when: inputPanel.active
            PropertyChanges { target: inputPanel; y: root.height - inputPanel.height }
        }
        transitions: Transition {
            from: ""; to: "visible"; reversible: true
            NumberAnimation { properties: "y"; duration: 200; easing.type: Easing.InOutQuad }
        }
    }

    FileDialog {
        id: importDialog
        // STUB — Faz 2 wires this to ImportFacade.importFile().
    }

    // Placeholder dialog (real FileDialog requires Qt.labs.platform; keep API stable).
    Item { id: FileDialog; property bool visible: false; function open() { /* TODO */ } }
}
