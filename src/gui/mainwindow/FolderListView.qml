// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Window 2.15

Item {
    property int itemHeight: 30
    width: 640
    height: 480
    visible: true
    signal itemChanged(int index, string name)

    property alias model: folderListView.model

    ListModel {
        id: folderModel
    }

    ListView {
        anchors.fill: parent
        id: folderListView
        model: folderModel
        delegate: Rectangle {
            width: parent.width
            height: itemHeight
            color: "white"
            Text {
                text: model.name
                anchors.centerIn: parent
                font.pixelSize: 14
            }
            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    folderListView.currentIndex = index
                }
                onEntered: {
                    parent.color = "red"
                }
                onExited: {
                    parent.color = "white"
                }
            }
        }
        onCurrentItemChanged: {
            var index = folderListView.currentIndex
            itemChanged(index, folderModel.get(index).name) // 发出 itemChanged 信号
        }
    }
}
