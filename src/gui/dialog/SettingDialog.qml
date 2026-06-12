// SPDX-FileCopyrightText: 2024-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import QtQml.Models 2.15
import org.deepin.dtk.settings 1.0 as Settings
import VNote 1.0
import org.deepin.dtk 1.0

Settings.SettingsDialog {
    id: control

    flags: Qt.Window | Qt.WindowCloseButtonHint
    modality: Qt.WindowModal
    height: 548
    width: 664

    config: QtObject {
        id: settingConfig

        property int audioSource: 1
    }

    // Override the default footer to use our own "Restore Defaults" button
    contentView.footer: Item {
        width: parent.width
        height: 60

        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            text: qsTr("Restore Defaults")

            onClicked: {
                // Default value is 1 (Microphone), sync with JSON setting "default": 1
                source.value = 1
            }
        }
    }

    groups: [
        Settings.SettingsGroup {
            key: "Basic"
            name: qsTr("Basic")

            children: [
                Settings.SettingsGroup {
                    key: "audioSource"
                    name: qsTr("Audio Source")

                    background: Settings.ContentBackground {
                        color: palette.base
                    }

                    Settings.SettingsOption {
                        id: source
                        key: "audioSource"

                        Component.onCompleted: {
                            value = VNoteMainManager.loadAudioSource();
                        }
                        onValueChanged: {
                            VNoteMainManager.changeAudioSource(value);
                        }

                        ButtonGroup {
                            buttons: column.children
                        }

                        Column {
                            id: column

                            RadioButton {
                                checked: Settings.SettingsOption.value === 0
                                font: DTK.fontManager.t8
                                height: 30
                                text: qsTr("Internal")

                                onClicked: Settings.SettingsOption.value = 0
                            }

                            RadioButton {
                                checked: Settings.SettingsOption.value === 1
                                font: DTK.fontManager.t8
                                height: 30
                                text: qsTr("Microphone")

                                onClicked: Settings.SettingsOption.value = 1
                            }
                        }
                    }
                }
            ]
        }
    ]
}
