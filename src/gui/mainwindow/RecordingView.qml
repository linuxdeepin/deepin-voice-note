import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import org.deepin.dtk 1.0
import VNote 1.0

Item {
    id: rootItem

    property int iconSize: 24
    property bool isRecording: true
    property string time: "00:00"

    signal pauseRecording
    signal stopRecording

    implicitHeight: 42
    implicitWidth: 364
    visible: false

    onVisibleChanged: {
        curves.startRecording();
    }

    Rectangle {
        anchors.fill: parent
        border.color: DTK.themeType === ApplicationHelper.LightType ? "#0A000000" : "#19FFFFFF"
        border.width: 1
        color: DTK.themeType === ApplicationHelper.LightType ? "#7FF4F4F4" : "#0A0A10"
        radius: 12
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        ToolButton {
            id: pauseBtn

            height: rootItem.height
            icon.height: iconSize
            icon.name: isRecording ? "recording_pause" : "recording_start"
            icon.width: iconSize
            spacing: 0
            width: rootItem.height

            onClicked: {
                pauseRecording();
                curves.pauseRecording();
            }
        }

        RecordingCurves {
            id: curves

            height: 24
            width: 180
        }

        Label {
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
                curves.stopRecording();
                stopRecording();
                rootItem.visible = false;
                time = "00:00";
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
