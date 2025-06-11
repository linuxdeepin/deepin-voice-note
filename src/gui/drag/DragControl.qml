import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.15
import org.deepin.dtk 1.0

Window {
    property int folderItemWidth: 200
    property var imageSource: ""
    property bool isFolder: false
    property var itemColor: "#99F7F7F7"
    property int itemHeight: 30
    property var itemNumber: 0
    property var itemText: ""
    property int noteItemWidth: 240

    color: "transparent"
    // color: "green"
    //设置无标题栏
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.WindowDoesNotAcceptFocus | Qt.WindowTransparentForInput
    height: 100
    visible: false
    width: 300

    Rectangle {
        id: rect2

        anchors.centerIn: parent
        anchors.horizontalCenterOffset: -2
        border.color: "#0D000000"
        border.width: 1
        color: itemColor
        height: itemHeight
        radius: 8
        rotation: 4
        visible: itemNumber > 1
        width: noteItemWidth

        BoxShadow {
            anchors.fill: rect2
            cornerRadius: rect2.radius
            hollow: true
            shadowBlur: 8
            shadowColor: Qt.rgba(0, 0, 0, 0.1)
            shadowOffsetX: 0
            shadowOffsetY: 4
            spread: 0
        }
    }

    Rectangle {
        id: rect3

        anchors.centerIn: parent
        anchors.horizontalCenterOffset: -2
        border.color: "#0D000000"
        border.width: 1
        color: itemColor
        height: itemHeight
        radius: 8
        rotation: -4
        visible: itemNumber > 1
        width: noteItemWidth
        BoxShadow {
            anchors.fill: rect3
            cornerRadius: rect3.radius
            hollow: true
            shadowBlur: 8
            shadowColor: Qt.rgba(0, 0, 0, 0.1)
            shadowOffsetX: 0
            shadowOffsetY: 4
            spread: 0
        }
    }

    Rectangle {
        id: rect1

        anchors.centerIn: parent
        anchors.horizontalCenterOffset: -2
        border.color: "#0D000000"
        border.width: 1
        color: itemColor
        height: itemHeight
        radius: 8
        visible: !isFolder
        width: noteItemWidth

        BoxShadow {
            anchors.fill: rect1
            cornerRadius: rect1.radius
            hollow: true
            shadowBlur: 0
            shadowColor: Qt.rgba(0, 0, 0, 0.1)
            shadowOffsetX: 0
            shadowOffsetY: 4
            spread: 0
        }

        Text {
            id: text1

            anchors.centerIn: parent
            color: "#000000"
            elide: Text.ElideRight
            font: DTK.fontManager.t6
            text: itemText
            width: noteItemWidth - 20
        }
    }

    Rectangle {
        id: rect4

        // anchors.right: parent.right
        // anchors.top: parent.top
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#FF674A" }   // 渐变起始颜色
            GradientStop { position: 1.0; color: "#EC5783" }  // 渐变结束颜色
        }
        height: 20
        radius: 10
        visible: itemNumber > 1
        width: 20
        x: rect1.x + rect1.width - width / 2
        y: rect1.y - height + 2

        BoxShadow {
            anchors.fill: rect4
            cornerRadius: rect4.radius
            hollow: true
            shadowBlur: 0
            shadowColor: Qt.rgba(0, 0, 0, 0.2)
            shadowOffsetX: 0
            shadowOffsetY: -1
            spread: 0
        }

        Text {
            id: text2

            anchors.centerIn: rect4
            color: "#FFFFFF"
            font: DTK.fontManager.t6
            text: itemNumber
        }
    }

    Rectangle {
        id: floderRect

        anchors.centerIn: parent
        color: "transparent"
        height: 30
        visible: isFolder
        width: folderItemWidth

        Image {
            id: folderImage

            source: imageSource
        }
    }
}
