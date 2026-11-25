// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import org.deepin.dtk 1.0 as D
import org.deepin.dtk.style 1.0 as DS
import VNote 1.0

// 与控制中心 DccWindow.qml 一致的窗口背后毛玻璃混合色
// 弱 GPU 平台（ARM/LoongArch）降级为纯色背景，避免全窗模糊合成开销
Item {
    id: control

    property var windowControl
    readonly property bool useBlur: !VNoteMainManager.isWeakGpuPlatform()

    anchors.fill: parent

    D.StyledBehindWindowBlur {
        id: blurItem
        anchors.fill: parent
        control: control.windowControl
        visible: control.useBlur
        blendColor: {
            if (valid) {
                return DS.Style.control.selectColor(control.windowControl ? control.windowControl.palette.window : undefined,
                                                    Qt.rgba(1, 1, 1, 0.8),
                                                    Qt.rgba(0.06, 0.06, 0.06, 0.8))
            }
            return DS.Style.control.selectColor(undefined,
                                                DS.Style.behindWindowBlur.lightNoBlurColor,
                                                DS.Style.behindWindowBlur.darkNoBlurColor)
        }
    }

    // 弱 GPU 平台降级：纯色背景替代全窗模糊
    Rectangle {
        id: fallbackItem
        anchors.fill: parent
        visible: !control.useBlur
        color: DS.Style.control.selectColor(control.windowControl ? control.windowControl.palette.window : undefined,
                                            DS.Style.behindWindowBlur.lightNoBlurColor,
                                            DS.Style.behindWindowBlur.darkNoBlurColor)
    }
}
