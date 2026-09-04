// SPDX-FileCopyrightText: 2024-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Layouts 1.15
import VNote 1.0
import org.deepin.dtk 1.0
import "../" as VNoteComponents

TitleBar {
    id: titleBar

    property bool isInitialInterface: true
    property bool isPlaying: false
    property bool isRecording: false
    property bool isRecordingAudio: false  
    property bool isSearching: false
    property bool isVoiceToText: false
    property bool recorderBtnEnable: true
    property bool recordBtnEnabled: recorderBtnEnable && !isPlaying && !isSearching && !isRecordingAudio && !isVoiceToText

    signal titleOpenSetting

    enableInWindowBlendBlur: false
    separatorVisible: false

    anchors.fill: parent

    background: Rectangle {
        color: DTK.themeType === ApplicationHelper.LightType ? "white" : "#242424"
    }
    content: Item {
        Component.onCompleted: {
            parent.Layout.leftMargin = 0;
        }
    }
    menu: TitleBarMenu {
        Accessible.name: "TitleBarMenu"
        Accessible.role: Accessible.PopupMenu
        id: tMenu

        onOpenPrivacy: {
            VNoteMainManager.showPrivacy();
        }
        onOpenSetting: {
            titleBar.titleOpenSetting();
        }
    }

}
