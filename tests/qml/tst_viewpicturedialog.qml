// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtTest 1.3
import "../../src/gui/dialog" as Dialogs

TestCase {
    name: "ViewPictureDialogGeometry"
    when: true

    Component {
        id: previewComponent
        Dialogs.ViewPictureDialog {}
    }

    function test_close_button_stays_with_image_after_resize() {
        const source = Qt.resolvedUrl("../../assets/deepin-voice-note/voice-notes/zh_CN/fig/picture.png");
        const preview = previewComponent.createObject(null, {filePath: source.toString(), width: 800, height: 600, visible: true});
        verify(preview !== null);
        const image = findChild(preview, "PreviewImage");
        const close = findChild(preview, "PreviewCloseButton");
        verify(image !== null);
        verify(close !== null);
        tryCompare(image, "status", Image.Ready);
        tryVerify(function() { return image.y >= 0 && image.y + image.height <= preview.height; }, 1000, "preview content should be laid out");

        verify(image.width <= preview.width * 0.8 + 1, "image width " + image.width + " window width " + preview.width);
        verify(image.height <= preview.height * 0.8 + 1, "image height " + image.height + " window height " + preview.height);
        compare(Math.round(image.width / image.height), Math.round(image.sourceSize.width / image.sourceSize.height));
        compare(close.x, Math.max(0, Math.min(preview.width - close.width, image.x + image.width - close.width / 2)));
        verify(Math.abs(close.y + close.height / 2 - image.y) <= 1, "close button should straddle the image's top edge");

        preview.width = 500;
        preview.height = 320;
        tryVerify(function() { return image.y >= 0 && image.y + image.height <= preview.height; }, 1000, "resized preview content should be laid out");
        verify(close.x >= 0 && close.x + close.width <= preview.width);
        verify(close.y >= 0 && close.y + close.height <= preview.height);
        verify(Math.abs(close.y + close.height / 2 - image.y) <= 1, "close button should follow the image after resize");
        preview.close();
        preview.destroy();
    }
}
