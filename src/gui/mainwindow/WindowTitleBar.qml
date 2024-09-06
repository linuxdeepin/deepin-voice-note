import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import org.deepin.dtk 1.0

TitleBar {
    id: titleBar

    property bool recorderBtnEnable: true

    signal createNote
    signal insertImage
    signal startRecording

    Layout.fillWidth: true
    content: titleBarContent
    height: 40

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
                companyLogo: globalVariant.appIconName
                description: qsTr("Music is a local music player with beautiful design and simple functions.")
                productIcon: globalVariant.appIconName
                productName: qsTr("Music")
                version: qsTr("Version:") + "%1".arg(Qt.application.version)
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

    Component {
        id: titleBarContent

        RowLayout {
            id: titleRowLayout

            anchors.fill: parent

            ToolButton {
                id: newNoteBtn

                Layout.alignment: Qt.AlignLeft
                hoverEnabled: true
                icon.name: "new_note"

                onClicked: {
                    createNote();
                }

                ToolTip {
                    text: qsTr("Previous page")
                }
            }

            Item {
                Layout.fillWidth: true
            }

            ToolButton {
                id: recordBtn

                enabled: recorderBtnEnable
                hoverEnabled: true
                icon.name: "record"

                onClicked: {
                    startRecording();
                }

                ToolTip {
                    text: qsTr("Start recording")
                }
            }

            ToolButton {
                id: insImgBtn

                hoverEnabled: true
                icon.name: "img"

                onClicked: {
                    insertImage();
                }

                ToolTip {
                    text: qsTr("Insert picture")
                }
            }
        }
    }
}
