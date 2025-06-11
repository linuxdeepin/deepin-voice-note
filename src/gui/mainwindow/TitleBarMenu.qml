import QtQuick 2.15
import VNote 1.0
import org.deepin.dtk 1.0

Menu {
    signal openPrivacy
    signal openSetting

    width: 200
    x: 0
    y: 0

    MenuItem {
        id: settingsControl

        text: qsTr("Settings")

        onTriggered: {
            openSetting();
            // if (settingDlgLoader.status === Loader.Null)
            //     settingDlgLoader.setSource("../dialog/SettingDialog.qml");
            // if (settingDlgLoader.status === Loader.Ready)
            //     settingDlgLoader.item.show();
        }
    }

    MenuItem {
        id: privacyBtn

        text: qsTr("Privacy Policy")

        onTriggered: {
            openPrivacy();
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

    QuitAction {
        onTriggered: VNoteMainManager.forceExit()
    }
}
