import QtQuick 2.15
import QtQuick.Layouts
import org.deepin.dtk 1.0

Item {
    id: rootWindow

    property bool isLoad: true

    signal createFolder

    function loadFinished(hasFile) {
        isLoad = false;
        rootWindow.visible = !hasFile;
    }

    WaterProgressBar {
        anchors.centerIn: parent
        value: 56
        visible: isLoad
    }

    TitleBar {
        id: title

        Layout.fillWidth: true
        anchors.top: rootWindow.top
        height: 40
    }

    ColumnLayout {
        Layout.fillHeight: true
        Layout.fillWidth: true
        anchors.centerIn: parent
        spacing: 0
        visible: !isLoad

        Image {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            height: 168
            source: "qrc:/icon/no_content.dci"
            sourceSize: Qt.size(168, 168)
            width: 168
        }

        Text {
            id: description

            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            text: qsTr("After creating a new notepad, you can start recording voice and text")
            wrapMode: Text.WordWrap
        }

        RecommandButton {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Layout.topMargin: 10
            text: qsTr("Create Notebook")
            width: 300

            onClicked: {
                rootWindow.createFolder();
            }
        }
    }
}
