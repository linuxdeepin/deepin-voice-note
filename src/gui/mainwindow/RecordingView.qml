import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import VNote 1.0
import org.deepin.dtk 1.0
import "../" as VNoteComponents

FocusScope {
    id: rootItem
    focus: visible

    property int btnSize: ApplicationHelper.sizeMode === ApplicationHelper.CompactMode ? 24 : 30
    property int iconSize: ApplicationHelper.sizeMode === ApplicationHelper.CompactMode ? 56 : 72
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

    implicitHeight: ApplicationHelper.sizeMode === ApplicationHelper.CompactMode ? 56 : 70
    visible: false

    onVisibleChanged: {
        if (visible) {
            curves.startRecording();
            pauseBtn.forceActiveFocus();
        } else {
            curves.stopRecording();
        }
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
        implicitHeight: ApplicationHelper.sizeMode === ApplicationHelper.CompactMode ? 36 : 42
        implicitWidth: ApplicationHelper.sizeMode === ApplicationHelper.CompactMode ? 300 : 364
        radius: 12
    }

    RowLayout {
        anchors.centerIn: back
        implicitHeight: ApplicationHelper.sizeMode === ApplicationHelper.CompactMode ? 36 : 42
        implicitWidth: ApplicationHelper.sizeMode === ApplicationHelper.CompactMode ? 300 : 364
        spacing: 10

        VNoteComponents.VNoteToolButton {
            Accessible.name: "PauseButton"
            Accessible.ignored: false
            Accessible.role: Accessible.Button
            id: pauseBtn

            icon.height: iconSize
            icon.name: isRecording ? "recording_pause" : "recording_start"
            icon.width: iconSize
            implicitHeight: btnSize
            implicitWidth: btnSize
            KeyNavigation.tab: stopBtn
            KeyNavigation.backtab: stopBtn

            background: Item {
            }

            onClicked: {
                pauseRecording();
                curves.pauseRecording();
            }
        }

        RecordingCurves {
            id: curves

            height: ApplicationHelper.sizeMode === ApplicationHelper.CompactMode ? 20 : 24
            width: ApplicationHelper.sizeMode === ApplicationHelper.CompactMode ? 150 : 180
        }

        Label {
            id: duation

            text: time
            width: ApplicationHelper.sizeMode === ApplicationHelper.CompactMode ? 54 : 66
        }

        VNoteComponents.VNoteToolButton {
            Accessible.name: "StopButton"
            Accessible.ignored: false
            Accessible.role: Accessible.Button
            id: stopBtn

            icon.height: iconSize
            icon.name: "recording_stop"
            icon.width: iconSize
            implicitHeight: btnSize
            implicitWidth: btnSize
            KeyNavigation.tab: pauseBtn
            KeyNavigation.backtab: pauseBtn

            background: Item {
            }

            onClicked: {
                stop();
            }
        }
    }

    MouseArea {
        anchors.fill: back
        onPressed: function(mouse) {
            var p1 = pauseBtn.mapFromItem(this, mouse.x, mouse.y);
            var p2 = stopBtn.mapFromItem(this, mouse.x, mouse.y);
            var onPause = pauseBtn.contains(p1);
            var onStop = stopBtn.contains(p2);
            if (onPause || onStop) {
                mouse.accepted = false;
                return;
            }
            rootItem.forceActiveFocus();
            mouse.accepted = true;
        }
    }

    Connections {
        target: VoiceRecoderHandler

        onUpdateWave: function(max) {
            curves.updateVolume(max);
        }
    }
}
