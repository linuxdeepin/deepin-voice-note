import QtQuick 2.15
import QtQuick.Layouts
import org.deepin.dtk 1.0


Item {
    id: rootWindow
    property int selectSize: 0
    visible: false

    Rectangle {
        anchors.fill: parent

        ColumnLayout {
            anchors.centerIn: parent
            Layout.fillWidth: true
            Layout.fillHeight: true

            Image {
                id: image
                Layout.alignment: Qt.AlignHCenter
                source: "qrc:/icon/multiple_choice.dci"
            }

            Text {
                id: description
                Layout.alignment: Qt.AlignHCenter
                wrapMode: Text.WordWrap
                text: qsTr("%1 note selected").arg(selectSize)
            }

            FloatingPanel {
                implicitWidth: 188
                implicitHeight: 52
                radius: 6
                contentItem: RowLayout {
                    spacing: 0
                    ToolButton { icon.name: "action_newfolder"; text: "文件夹"; implicitWidth: 40; implicitHeight: 40 }
                    ToolButton { icon.name: "action_copy"; text: "复制"; implicitWidth: 40; implicitHeight: 40 }
                    ToolButton { icon.name: "action_share"; text: "分享"; implicitWidth: 40; implicitHeight: 40 }
                    ToolButton { icon.name: "action_compress"; text: "压缩"; implicitWidth: 40; implicitHeight: 40 }
                }
            }
        }
    }
}
