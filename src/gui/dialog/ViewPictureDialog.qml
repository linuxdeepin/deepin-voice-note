// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import org.deepin.dtk 1.0

Window {
    id: root

    property string filePath: ""
    readonly property string imageSource: {
        if (filePath.length === 0) {
            return "";
        }
        if (filePath.indexOf("file://") === 0 || filePath.indexOf("qrc:/") === 0) {
            return filePath;
        }
        return "file://" + filePath;
    }

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
        objectName: "PreviewImage"

        readonly property real fitScale: sourceSize.width > 0 && sourceSize.height > 0
            ? Math.min(1, root.width * 0.8 / sourceSize.width, root.height * 0.8 / sourceSize.height)
            : 1

        anchors.centerIn: backRect
        fillMode: Image.PreserveAspectFit
        height: sourceSize.height * fitScale
        source: root.imageSource
        width: sourceSize.width * fitScale
        z: 100
    }

    Rectangle {
        id: back

        anchors.fill: img
        color: back.palette.window
        radius: 12
    }

    FloatingButton {
        id: closeBtn
        objectName: "PreviewCloseButton"

        Accessible.name: "ClosePictureView"
        checked: false
        x: Math.max(0, Math.min(root.width - width, img.x + img.width - width / 2))
        y: Math.max(0, Math.min(root.height - height, img.y - height / 2))
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
