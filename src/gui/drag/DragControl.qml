import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Window 2.11

Window {
    width: 250
    height: 43
    visible: false
    //设置无标题栏
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.WindowDoesNotAcceptFocus | Qt.WindowTransparentForInput
    color: "transparent"
    property int noteItemWidth: 240
    property int folderItemWidth: 200
    property int itemHeight: 30
    property bool isFolder: false
    property var imageSource: ""

    property var itemText: ""
    property var itemNumber: 0
    Rectangle {
        id: rect2
        visible: itemNumber > 1
        rotation: 3
        anchors.centerIn: parent
        anchors.horizontalCenterOffset: -2
        width: noteItemWidth
        height: itemHeight
        color: "#99F7F7F7"
        border.color: "#0D000000"
        border.width: 1
        radius: 8
    }
    Rectangle {
        id: rect3
        visible: itemNumber > 1
        rotation: -3
        anchors.centerIn: parent
        anchors.horizontalCenterOffset: -2
        width: noteItemWidth
        height: itemHeight
        color: "#99F7F7F7"
        border.color: "#0D000000"
        border.width: 1
        radius: 8
    }
    Rectangle {
        id: rect1
        anchors.centerIn: parent
        anchors.horizontalCenterOffset: -2
        visible: !isFolder
        width: noteItemWidth
        height: itemHeight
        color: "#99F7F7F7"
        border.color: "#0D000000"
        border.width: 1
        radius: 8
        
        Text {
            id: text1
            anchors.centerIn: parent
            text: itemText
            font.pixelSize: 14
            color: "#000000"
        }
    }
    //在右上角画一个红色圆形rectangle,中间写数字
    Rectangle {
        id: rect4
        visible: itemNumber > 1
        anchors.top: parent.top
        anchors.right: parent.right
        width: 20
        height: 20
        color: "#FF0000"
        radius: 10

        Text {
            id: text2
            anchors.centerIn: rect4
            text: itemNumber
            font.pixelSize: 14
            color: "#FFFFFF"
        }
    }

    Rectangle {
        id: floderRect
        visible: isFolder
        anchors.centerIn: parent
        width: folderItemWidth
        height: 30
        color: "transparent"
        Image {
            id: folderImage
            source: imageSource
        }
    }
}