// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11

import org.deepin.dtk 1.0

DialogWindow {
    property int dialogWidth: 360
    property int dialogHeight: 345
    property string noteName: ""
    property var floderList: {}
    width: dialogWidth
    height: dialogHeight
    title: qsTr("Move Note")
    ColumnLayout {
        width: parent.width
        Label {
            Layout.alignment: Qt.AlignHCenter
            font: DTK.fontManager.t5
            text: qsTr("Move note %1 to:").arg(noteName)
        }
        ScrollBar {
            id: vbar
            orientation: Qt.Vertical
            size: frame.height / content.height
            position: 0.4
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.bottom: parent.bottom
        }
        Rectangle {
            border.color: "#cacaca"
            width: 348
            height: 226
            Flickable {
                width: 348
                height: 226
                contentWidth: parent.width
                contentHeight: 500
                clip: true

                ScrollBar.vertical: ScrollBar { }

                ListModel {
                    id: floderListModel
                    ListElement {name: "name1"}
                    ListElement {name: "name2"}
                    ListElement {name: "name3"}
                }

                ListView {
                    id: moveFolderListView
                    model: floderListModel
                    anchors.fill: parent
                    clip: true
                    delegate: Rectangle {
                        width: 336
                        height: 30
                        Text {
                            anchors.centerIn: parent
                            text: model.name
                        }
                    }
                    // Layout.fillWidth: true
                    // Layout.fillHeight: true
                    onCurrentIndexChanged: {
                        accpetButton.enabled = true
                    }
                }
            }
        }
        RowLayout {
            Layout.alignment: Qt.AlignBottom | Qt.AlignHCenter
            Layout.bottomMargin: 10
            Layout.topMargin: 10
            Layout.fillWidth: true
            Button {
                text: "取消"
                Layout.preferredWidth: 175
                onClicked: close()
            }
            Item {Layout.fillWidth: true}
            RecommandButton {
                id: accpetButton
                text: "确定"
                enabled: false
                Layout.preferredWidth: 175
                Layout.alignment: Qt.AlignRight
            }
        }
    }
}
