import QtQuick 2.15
import QtQuick.Layouts 1.15
import org.deepin.dtk 1.0

Item {
    id: rootWindow

    property int selectSize: 0

    signal deleteNote
    signal moveNote
    signal saveAudio
    signal saveNote

    visible: false

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
                property int fontSize: 6

                Layout.topMargin: 20
                implicitHeight: 52
                implicitWidth: 188
                radius: 6

                contentItem: RowLayout {
                    spacing: 0

                    ToolButton {
                        icon.name: "move_note"
                        implicitHeight: 40
                        implicitWidth: 40
                        text: qsTr("Move")

                        onClicked: {
                            moveNote();
                        }
                    }

                    ToolButton {
                        icon.name: "save_note"
                        implicitHeight: 40
                        implicitWidth: 48
                        leftPadding: 0
                        rightPadding: 0
                        text: qsTr("Save Note")

                        onClicked: {
                            saveNote();
                        }
                    }

                    ToolButton {
                        icon.name: "save_audio"
                        implicitHeight: 40
                        implicitWidth: 48
                        leftPadding: 0
                        rightPadding: 0
                        text: qsTr("Save Voice")

                        onClicked: {
                            saveAudio();
                        }
                    }

                    ToolButton {
                        icon.name: "delete"
                        implicitHeight: 40
                        implicitWidth: 40
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
