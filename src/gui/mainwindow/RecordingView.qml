import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import org.deepin.dtk 1.0
import VNote 1.0

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
        spacing: 0

        ToolButton {
            id: pauseBtn

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

        RecordingCurves {
            id: curves

            height: 24
            width: 180
        }

        Text {
            id: duation

            text: time
            width: 66
        }

        ToolButton {
            id: stopBtn

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

    Connections {
        target: VoiceRecoderHandler

        onUpdateWave: {
            curves.updateVolume(max);
        }
    }
}
