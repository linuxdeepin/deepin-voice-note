import QtQuick 2.15
import QtQuick.Layouts
import org.deepin.dtk 1.0

Item {
    id: rootWindow

    property bool isLoad: true
    signal createFolder()

    WaterProgressBar {
        anchors.centerIn: parent
        visible: isLoad
        value: 56
    }

    TitleBar {
        id: title
        anchors.top: rootWindow.top
        Layout.fillWidth: true
        height: 40
    }

    ColumnLayout {
        spacing: 0
        anchors.centerIn: parent
        Layout.fillWidth: true
        Layout.fillHeight: true
        visible: !isLoad

        Image {
            width: 168
            height: 168
            source: "qrc:/icon/no_content.dci"
            sourceSize: Qt.size(168, 168)
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
        }

        Text {
            id: description
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            text: qsTr("After creating a new notepad, you can start recording voice and text")
        }

        RecommandButton {
            width: 300
            Layout.topMargin: 10
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            text: qsTr("Create Notebook")

            onClicked: {
                rootWindow.createFolder()
            }
        }
    }

    function loadFinished(hasFile) {
        if (hasFile) {
            rootWindow.visible = false
        } else {
            isLoad = false
        }
    }
    }
