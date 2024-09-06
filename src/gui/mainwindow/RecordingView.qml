import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import org.deepin.dtk 1.0

Item {
    id: rootItem

    property int iconSize: 24
    property string time: "00:00:00"

    signal pauseRecording
    signal stopRecording

    implicitHeight: 42
    implicitWidth: 364
    visible: false

    Rectangle {
        anchors.fill: parent
        border.color: "#0A000000"
        border.width: 1
        color: "#7FF4F4F4"
        radius: 12
    }

    RowLayout {
        anchors.fill: parent
        spacing: 10

        ToolButton {
            id: pauseBtn

            Layout.leftMargin: 10
            height: rootItem.height
            icon.height: iconSize
            icon.name: "recording_pause"
            icon.width: iconSize
            spacing: 0
            width: rootItem.height

            onClicked: {
                pauseRecording();
            }
        }

        Rectangle {
            width: 180
        }

        Text {
            id: duation

            text: time
        }

        ToolButton {
            id: stopBtn

            Layout.rightMargin: 10
            height: rootItem.height
            icon.height: iconSize
            icon.name: "recording_stop"
            icon.width: iconSize
            width: rootItem.height

            onClicked: {
                stopRecording();
                rootItem.visible = false;
            }
        }
    }
}
