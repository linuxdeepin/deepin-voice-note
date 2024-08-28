import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import org.deepin.dtk 1.0

TitleBar {
    signal createNote()
    signal startRecording()
    signal insertImage()

    id: titleBar
    Layout.fillWidth: true
    height: 40

    content: titleBarContent

    Component {
        id: titleBarContent

        RowLayout {
            id: titleRowLayout
            anchors.fill: parent

            ToolButton {
                id: newNoteBtn
                icon.name: "new_note"
                hoverEnabled: true
                Layout.alignment: Qt.AlignLeft
                ToolTip {
                    text: qsTr("Previous page")
                }
                onClicked: {
                    createNote()
                }
            }

            Item { Layout.fillWidth: true }

            ToolButton {
                id: recordBtn
                icon.name: "record"
                hoverEnabled: true
                ToolTip {
                    text: qsTr("Start recording")
                }
                onClicked: {
                    startRecording()
                }
            }
            ToolButton {
                id: insImgBtn
                icon.name: "img"
                hoverEnabled: true
                ToolTip {
                    text: qsTr("Insert picture")
                }
                onClicked: {
                    insertImage()
                }
            }
        }
    }
}
