// SPDX-FileCopyrightText: 2024-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Layouts 1.15
import org.deepin.dtk 1.0

Item {
    id: rootWindow

    property int selectSize: 0
    property bool deleteEnabled: true
    property bool moveEnabled: true
    property bool saveVoiceEnabled: true

    signal deleteNote
    signal moveNote
    signal saveAudio
    signal saveNote

    visible: false

    function setSaveVoiceEnabled(enabled) {
        saveVoiceEnabled = enabled;
    }

    function setOperationEnabled(move, del) {
        moveEnabled = move;
        deleteEnabled = del;
    }

    Rectangle {
        anchors.fill: parent
        color: DTK.themeType === ApplicationHelper.LightType ? "white" : "#242424"

        ColumnLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
            anchors.centerIn: parent

            Image {
                id: image

                Layout.alignment: Qt.AlignHCenter
                source: "qrc:/icon/multiple_choice.dci"
            }

            Label {
                id: description

                Layout.alignment: Qt.AlignHCenter
                text: qsTr("%1 note selected").arg(selectSize)
                wrapMode: Text.WordWrap
            }

            FloatingPanel {
                id: operationPanel
                property font actionFont: DTK.fontManager.t8

                Layout.topMargin: 20
                implicitHeight: 52
                implicitWidth: 188
                radius: 6

                contentItem: RowLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 0

                    ToolButton {
                        Accessible.name: "MoveButton"
                        Accessible.ignored: false
                        Accessible.role: Accessible.Button
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        display: AbstractButton.TextUnderIcon
                        icon.name: "move_note"
                        icon.width: 16
                        icon.height: 16
                        font: operationPanel.actionFont
                        leftPadding: 0
                        rightPadding: 0
                        topPadding: 0
                        bottomPadding: 0
                        enabled: moveEnabled
                        text: qsTr("Move")

                        onClicked: {
                            moveNote();
                        }
                    }

                    ToolButton {
                        Accessible.name: "SaveNoteButton"
                        Accessible.ignored: false
                        Accessible.role: Accessible.Button
                        Layout.preferredWidth: 48
                        Layout.preferredHeight: 40
                        display: AbstractButton.TextUnderIcon
                        icon.name: "save_note"
                        icon.width: 16
                        icon.height: 16
                        font: operationPanel.actionFont
                        leftPadding: 0
                        rightPadding: 0
                        topPadding: 0
                        bottomPadding: 0
                        text: qsTr("Save Note")

                        onClicked: {
                            saveNote();
                        }
                    }

                    ToolButton {
                        Accessible.name: "SaveVoiceButton"
                        Accessible.ignored: false
                        Accessible.role: Accessible.Button
                        Layout.preferredWidth: 48
                        Layout.preferredHeight: 40
                        display: AbstractButton.TextUnderIcon
                        icon.name: "save_audio"
                        icon.width: 16
                        icon.height: 16
                        font: operationPanel.actionFont
                        leftPadding: 0
                        rightPadding: 0
                        topPadding: 0
                        bottomPadding: 0
                        text: qsTr("Save Voice")
                        enabled: saveVoiceEnabled

                        onClicked: {
                            saveAudio();
                        }
                    }

                    ToolButton {
                        Accessible.name: "DeleteButton"
                        Accessible.ignored: false
                        Accessible.role: Accessible.Button
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        display: AbstractButton.TextUnderIcon
                        icon.name: "delete"
                        icon.width: 16
                        icon.height: 16
                        font: operationPanel.actionFont
                        leftPadding: 0
                        rightPadding: 0
                        topPadding: 0
                        bottomPadding: 0
                        enabled: deleteEnabled
                        text: qsTr("Delete")

                        onClicked: {
                            deleteNote();
                        }
                    }
                }
            }
        }
    }
}
