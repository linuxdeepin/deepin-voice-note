import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import VNote 1.0
import org.deepin.dtk 1.0

Item {
    id: rootItem

    property int btnSize: 30
    property int iconSize: 72
    property bool isRecording: true
    property string time: "00:00:00"

    signal pauseRecording
    signal stopRecording

    function stop() {
        curves.stopRecording();
        stopRecording();
        rootItem.visible = false;
        time = "00:00:00";
    }

    implicitHeight: 70
    visible: false

    onVisibleChanged: {
        curves.startRecording();
    }

    Rectangle {
        id: backRectangle

        anchors.fill: parent

        gradient: Gradient {
            GradientStop {
                color: DTK.themeType === ApplicationHelper.LightType ? "#19FFFFFF" : "#19242424"
                position: 0.0
            }

            GradientStop {
                color: DTK.themeType === ApplicationHelper.LightType ? "white" : "#242424"
                position: 0.3
            }
        }
    }

    Rectangle {
        id: back

        anchors.bottom: backRectangle.bottom
        anchors.bottomMargin: 10
        anchors.horizontalCenter: parent.horizontalCenter
        border.color: DTK.themeType === ApplicationHelper.LightType ? "#0A000000" : "#19FFFFFF"
        border.width: 1
        color: DTK.themeType === ApplicationHelper.LightType ? "#7FF4F4F4" : "#0A0A10"
        implicitHeight: 42
        implicitWidth: 364
        radius: 12
    }

    RowLayout {
        anchors.centerIn: back
        implicitHeight: 42
        implicitWidth: 364
        spacing: 10

        ToolButton {
            id: pauseBtn

            icon.height: iconSize
            icon.name: isRecording ? "recording_pause" : "recording_start"
            icon.width: iconSize
            implicitHeight: btnSize
            implicitWidth: btnSize

            background: Item {
            }

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

            icon.height: iconSize
            icon.name: "recording_stop"
            icon.width: iconSize
            implicitHeight: btnSize
            implicitWidth: btnSize

            background: Item {
            }

            onClicked: {
                stop();
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
