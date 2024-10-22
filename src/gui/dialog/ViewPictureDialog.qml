import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import org.deepin.dtk 1.0

Window {
    id: root

    property string filePath: ""

    color: "transparent"
    flags: Qt.FramelessWindowHint
    height: Screen.height
    width: Screen.width

    MouseArea {
        anchors.fill: parent

        onClicked: {
            root.close();
        }
    }

    Rectangle {
        id: backRect

        anchors.fill: parent
        color: "#4D000000"
    }

    Image {
        id: img

        anchors.centerIn: backRect
        fillMode: Image.PreserveAspectFit
        source: "file:///" + filePath
        z: 100

        onStatusChanged: {
            if (status === Image.Ready) {
                if (img.sourceSize.width > (root.width * 0.8)) {
                    img.width = root.width * 0.8;
                } else {
                    img.width = img.sourceSize.width;
                }
                if (img.sourceSize.width > (root.width * 0.8)) {
                    img.height = root.height * 0.8;
                } else {
                    img.height = img.sourceSize.height;
                }
                // sourceSize.width = img.width;
                // sourceSize.height = img.height;
                closeBtn.x = (root.width + img.width - closeBtn.width) / 2;
                closeBtn.y = (root.height - img.height) / 2 - closeBtn.height;
            }
        }
    }

    Rectangle {
        id: back

        anchors.fill: img
        color: back.palette.window
        radius: 12
    }

    FloatingButton {
        id: closeBtn

        checked: false
        z: 100

        onClicked: {
            root.close();
        }

        icon {
            height: 24
            name: "view-close"
            width: 24
        }
    }
}
