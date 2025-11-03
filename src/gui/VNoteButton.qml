// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import org.deepin.dtk 1.0

/**
 * VNoteButton - 修复 MIPS 平台键盘事件的 Button
 * 
 * Qt5 的 Button 不会自动响应 Enter/Return 键，需要显式处理
 * Qt6 已修复此问题，但此代码在两个版本上都能正常工作
 */
Button {
    id: root
    
    activeFocusOnTab: true

    Keys.onReturnPressed: function(event) {
        if (enabled && !checked) {
            clicked();
            event.accepted = true;  // 阻止事件传播，避免 Qt6 重复触发
        }
    }

    Keys.onEnterPressed: function(event) {
        if (enabled && !checked) {
            clicked();
            event.accepted = true;  // 阻止事件传播，避免 Qt6 重复触发
        }
    }
}

