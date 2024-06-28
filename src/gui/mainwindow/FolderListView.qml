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
        property var lastCurrentIndex: -1
        id: folderListView
        model: folderModel
        delegate: Rectangle {
            width: parent.width
            height: itemHeight
            radius: 6
            color: index === folderListView.currentIndex ? "#33000000" : "white"
            Text {
                text: model.name
                anchors.centerIn: parent
                font.pixelSize: 14
            }
            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    if (folderListView.lastCurrentIndex != -1) {
                        if (folderListView.itemAtIndex(folderListView.lastCurrentIndex)) {
                            folderListView.itemAtIndex(folderListView.lastCurrentIndex).color = "white"
                        }
                    }

                    folderListView.currentIndex = index
                    parent.color = "#33000000"
                    folderListView.lastCurrentIndex = index
                }
                onEntered: {
                    if (folderListView.currentIndex == index) {
                        return
                    }
                    parent.color = "#1A000000"
                }
                onExited: {
                    if (folderListView.currentIndex == index) {
                        return
                    }
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
