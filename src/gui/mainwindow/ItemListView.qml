// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Window 2.15

Item {
    property int noteItemHeight: 50
    width: 640
    height: 480
    visible: true
    signal noteItemChanged(int index, string name)

    property alias model: itemModel

    ListModel {
        id: itemModel
    }

    ListView {
        id: itemListView
        anchors.fill: parent
        model: itemModel
        delegate: Rectangle {
            width: parent.width
            height: noteItemHeight
            color: model.color
            Text {
                text: model.name
                anchors.centerIn: parent
                color: "white"
                font.pixelSize: 20
            }
        }
        onCurrentItemChanged: {
            var index = folderListView.currentIndex
            noteItemChanged(index)
        }
    }
}
