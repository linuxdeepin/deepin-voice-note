// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import org.deepin.dtk 1.0 as D
import org.deepin.dtk.style 1.0 as DS

// 与控制中心 DccWindow.qml 一致的窗口背后毛玻璃混合色
D.StyledBehindWindowBlur {
    id: control

    property var windowControl

    anchors.fill: parent
    control: windowControl
    blendColor: {
        if (valid) {
            return DS.Style.control.selectColor(windowControl ? windowControl.palette.window : undefined,
                                                Qt.rgba(1, 1, 1, 0.8),
                                                Qt.rgba(0.06, 0.06, 0.06, 0.8))
        }
        return DS.Style.control.selectColor(undefined,
                                            DS.Style.behindWindowBlur.lightNoBlurColor,
                                            DS.Style.behindWindowBlur.darkNoBlurColor)
    }
}
