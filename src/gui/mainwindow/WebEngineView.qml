// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Window 2.15
import QtWebChannel 1.15
import QtWebEngine 1.15

import VNote 1.0

Item {
    visible: true

    WebEngineView {
        id: webView
        anchors.fill: parent

        WebChannel {
            id: webChannel
        }

        Component.onCompleted: {
            webChannel.registerObject("webobj", Webobj);
            webView.webChannel = webChannel
            var webPage = webobj.webPath()
            webView.url = Qt.resolvedUrl(webPage)
        }
    }
}
