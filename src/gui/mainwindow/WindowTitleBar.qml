import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15
import VNote 1.0
import org.deepin.dtk 1.0

TitleBar {
    id: titleBar

    property bool imageBtnEnable: true
    property bool isInitialInterface: true
    property bool isPlaying: false
    property bool isRecording: false
    property bool recorderBtnEnable: true
    property bool recordingHover: false

    signal createNote
    signal insertImage
    signal startRecording
    signal titleOpenSetting

    enableInWindowBlendBlur: false
    height: 40
    width: 0

    background: Rectangle {
        color: DTK.themeType === ApplicationHelper.LightType ? "white" : "#242424"
    }
    content: Item {
        Component.onCompleted: {
            parent.Layout.leftMargin = 0;
        }
    }
    menu: TitleBarMenu {
        id: tMenu

        onOpenPrivacy: {
            VNoteMainManager.showPrivacy();
        }
        onOpenSetting: {
            titleBar.titleOpenSetting();
        }
    }

    ToolButton {
        id: newNoteBtn

        anchors.left: titleBar.left
        anchors.leftMargin: 10
        anchors.verticalCenter: titleBar.verticalCenter
        enabled: !isPlaying
        hoverEnabled: !isInitialInterface
        icon.name: "new_note"

        onClicked: {
            createNote();
        }

        ToolTip {
            text: qsTr("Create Note")
            visible: newNoteBtn.hovered
        }
    }

    Rectangle {
        id: maskRect

        anchors.fill: recordBtn
        color: "transparent"
        visible: !recordBtn.enabled

        MouseArea {
            hoverEnabled: true

            onEntered: {
                recordingHover = true;
            }
            onExited: {
                recordingHover = false;
            }
        }
    }

    ToolButton {
        id: recordBtn

        anchors.right: insImgBtn.left
        anchors.rightMargin: 6
        anchors.verticalCenter: titleBar.verticalCenter
        enabled: recorderBtnEnable && imageBtnEnable && !isPlaying
        hoverEnabled: !isInitialInterface
        icon.name: "record"

        onClicked: {
            startRecording();
            isRecording = true;
        }

        ToolTip {
            text: recordBtn.enabled ? qsTr("Start recording") : qsTr("No recording device detected")
            visible: (recordBtn.hovered || recordingHover) && !isRecording && !isInitialInterface
        }
    }

    ToolButton {
        id: insImgBtn

        anchors.verticalCenter: titleBar.verticalCenter
        enabled: imageBtnEnable
        hoverEnabled: !isInitialInterface
        icon.name: "img"
        x: titleBar.__includedAreaX - recordBtn.width - 10

        onClicked: {
            insertImage();
        }

        ToolTip {
            text: qsTr("Insert picture")
            visible: insImgBtn.hovered
        }
    }
}
