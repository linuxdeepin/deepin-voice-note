import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import org.deepin.dtk 1.0

TitleBar {
    id: titleBar

    property bool imageBtnEnable: true
    property bool recorderBtnEnable: true
    property bool recordingHover: false

    signal createNote
    signal insertImage
    signal startRecording

    enableInWindowBlendBlur: false
    height: 40
    width: 0

    background: Rectangle {
        color: DTK.themeType === ApplicationHelper.LightType ? "white" : "black"
    }
    content: Item {
        Component.onCompleted: {
            parent.Layout.leftMargin = 0;
        }
    }
    menu: Menu {
        width: 200
        x: 0
        y: 0

        MenuItem {
            id: settingsControl

            text: qsTr("Settings")

            onTriggered: {
                if (settingDlgLoader.status === Loader.Null)
                    settingDlgLoader.setSource("../dialog/SettingDialog.qml");
                if (settingDlgLoader.status === Loader.Ready)
                    settingDlgLoader.item.show();
            }
        }

        MenuSeparator {
        }

        ThemeMenu {
            width: 200
        }

        MenuSeparator {
        }

        HelpAction {
        }

        AboutAction {
            aboutDialog: AboutDialog {
                companyLogo: "deepin-voice-note"
                description: qsTr("Voice Notes is a lightweight memo tool to make text notes and voice recordings.")
                productIcon: "deepin-voice-note"
                productName: qsTr("Voice Note")
                version: Qt.application.version
                websiteLink: DTK.deepinWebsiteLink
                websiteName: DTK.deepinWebsiteName

                header: DialogTitleBar {
                    enableInWindowBlendBlur: false
                }
            }
        }
    }

    Loader {
        id: settingDlgLoader

    }

    ToolButton {
        id: newNoteBtn

        anchors.left: titleBar.left
        anchors.leftMargin: 10
        anchors.verticalCenter: titleBar.verticalCenter
        hoverEnabled: true
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
        enabled: recorderBtnEnable && imageBtnEnable
        hoverEnabled: true
        icon.name: "record"

        onClicked: {
            startRecording();
        }

        ToolTip {
            text: recordBtn.enabled ? qsTr("Start recording") : qsTr("No recording device detected")
            visible: recordBtn.hovered || recordingHover
        }
    }

    ToolButton {
        id: insImgBtn

        anchors.verticalCenter: titleBar.verticalCenter
        enabled: imageBtnEnable
        hoverEnabled: true
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
