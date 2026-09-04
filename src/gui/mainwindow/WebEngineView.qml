// SPDX-FileCopyrightText: 2024-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtWebChannel 1.15
// Use Qt5 version 1.10 for compatibility with both Qt5 and Qt6
// Qt6 is backward compatible with Qt5 import syntax
import QtWebEngine 1.10
import Qt.labs.platform 1.1
import VNote 1.0
import "../dialog"
import org.deepin.dtk 1.0

Item {
    id: rootItem

    property bool hasScroll: false
    property bool initialVisible: false
    property bool isRecording: false
    property bool isRecordingAudio: false
    property bool isVoiceToText: false
    property bool noSearchResult: false
    property bool webVisible: true
    property bool summernoteVisible: true
    property alias titleBar: title
    // 所有录音入口共用这一组条件，避免工具栏、标题栏和 Ctrl+R
    // 在搜索、播放、转写、录音或没有可用音源时出现不一致。
    readonly property bool recordingAvailable:
        !initialVisible
        && webVisible
        && title.recordBtnEnabled
        && !VNoteMainManager.isInSearchMode()

    Timer {
        id: txtMenuToolbarTimer
        interval: 50
        repeat: false
        property int retryCount: 0
        readonly property int maxRetries: 8
        onTriggered: rootItem.passTxtMenuToToolbar(txtCtxMenu, retryCount)
    }

    Timer {
        id: tiptapToolbarSeparatorSyncTimer
        interval: 16
        repeat: false
        onTriggered: rootItem.syncTiptapToolbarSeparators()
    }

    // 累加可见子项 implicitHeight/height（来自 DTK 布局，不用硬编码行高）
    function txtMenuItemsHeight(menu) {
        var sum = 0;
        for (var i = 0; i < menu.count; ++i) {
            var item = menu.itemAt(i);
            if (!item || item.visible === false) {
                continue;
            }
            var itemH = item.height > 0 ? item.height : item.implicitHeight;
            if (itemH > 0) {
                sum += itemH;
            }
        }
        return sum;
    }

    function txtMenuEffectiveHeight(menu) {
        var height = menu.height;
        var itemsH = txtMenuItemsHeight(menu);
        height = Math.max(height, itemsH);
        height = Math.max(height, menu.implicitHeight);
        if (menu.contentItem) {
            height = Math.max(height, menu.contentItem.height);
            height = Math.max(height, menu.contentItem.implicitHeight);
        }
        return height;
    }

    // menu.height 常为占位值；与可见子项总高度偏差大则视为未就绪
    function isTxtMenuHeightReady(menu) {
        var height = menu.height;
        if (height <= 0 || !menu.visible) {
            return false;
        }
        var itemsH = txtMenuItemsHeight(menu);
        if (itemsH > 0 && height < itemsH * 0.85) {
            return false;
        }
        return true;
    }

    function toJsInt(value) {
        var n = Math.round(Number(value));
        return isFinite(n) ? n : 0;
    }

    function resetTiptapToolbarSeparatorGeometry() {
        toolbarSeparatorOverlay.toolbarTopY = titleBarHost.height;
        toolbarSeparatorOverlay.toolbarBottomY = titleBarHost.height + toolbarSeparatorOverlay.defaultToolbarHeight;
    }

    function scheduleTiptapToolbarSeparatorSync() {
        if (!TiptapChannel.tiptapEnabled || !webVisible || noSearchResult) {
            resetTiptapToolbarSeparatorGeometry();
            return;
        }
        tiptapToolbarSeparatorSyncTimer.restart();
    }

    function syncTiptapToolbarSeparators() {
        var tiptapView = tiptapLoader.item ? tiptapLoader.item.editor : null;
        if (!tiptapView || !TiptapChannel.tiptapEnabled || !webVisible || noSearchResult) {
            resetTiptapToolbarSeparatorGeometry();
            return;
        }

        tiptapView.runJavaScript(
            "(function(){var el=document.getElementById('toolbar-host');"
            + "if(!el)return null;var r=el.getBoundingClientRect();"
            + "return {top:r.top,bottom:r.bottom};})()",
            function(rect) {
                if (!rect || rect.top === undefined || rect.bottom === undefined) {
                    resetTiptapToolbarSeparatorGeometry();
                    return;
                }
                var top = Number(rect.top);
                var bottom = Number(rect.bottom);
                if (!isFinite(top) || !isFinite(bottom) || bottom <= top) {
                    resetTiptapToolbarSeparatorGeometry();
                    return;
                }
                toolbarSeparatorOverlay.toolbarTopY = tiptapLoader.y + top;
                toolbarSeparatorOverlay.toolbarBottomY = tiptapLoader.y + bottom;
            });
    }

    function consumeTiptapNativeZoomFactor(tiptapView) {
        if (!tiptapView || !TiptapChannel.tiptapEnabled) {
            return;
        }
        var factor = Number(tiptapView.zoomFactor);
        if (!isFinite(factor) || Math.abs(factor - 1.0) < 0.001) {
            return;
        }

        // 工具栏是应用 chrome，不参与正文缩放。若 QtWebEngine 仍触发
        // 整页 zoom，将本次 factor 转给 Tiptap 正文缩放并立即复位 WebEngine。
        var safeFactor = Math.max(0.1, Math.min(10.0, factor));
        tiptapView.runJavaScript(
            "window.__dvnTiptapApplyNativeZoomFactor"
            + "&& window.__dvnTiptapApplyNativeZoomFactor(" + safeFactor + ");");
        tiptapView.dvnResettingNativeZoom = true;
        tiptapView.zoomFactor = 1.0;
        Qt.callLater(function() {
            if (tiptapView) {
                tiptapView.dvnResettingNativeZoom = false;
            }
            rootItem.scheduleTiptapToolbarSeparatorSync();
        });
    }

    function clearTiptapInsertionSelection() {
        var tiptapView = tiptapLoader.item ? tiptapLoader.item.editor : null;
        if (tiptapView) {
            tiptapView.runJavaScript(
                "window.__dvnTiptapClearInsertionSelection"
                + "&& window.__dvnTiptapClearInsertionSelection();");
        }
    }

    function stableUrlList(urls) {
        var copied = [];
        if (!urls) {
            return copied;
        }
        for (var i = 0; i < urls.length; ++i) {
            copied.push(urls[i]);
        }
        return copied;
    }

    function insertImagesAtTiptapClientPoint(urls, x, y) {
        // DropEvent 是临时事件对象，runJavaScript callback 异步触发时
        // drop.urls 可能已经失效；必须先复制成稳定列表再进入回调。
        var imageUrls = stableUrlList(urls);
        if (imageUrls.length === 0) {
            return;
        }

        var tiptapView = tiptapLoader.item ? tiptapLoader.item.editor : null;
        if (!tiptapView) {
            VNoteMainManager.insertImages(imageUrls);
            return;
        }

        var safeX = toJsInt(x);
        var safeY = toJsInt(y);
        tiptapView.runJavaScript(
            "(function(){return !!(window.__dvnTiptapCaptureInsertionPointFromClient"
            + "&& window.__dvnTiptapCaptureInsertionPointFromClient("
            + safeX + "," + safeY + "));})()",
            function() {
                VNoteMainManager.insertImages(imageUrls);
            });
    }

    function passTxtMenuToToolbar(menu, retryCount) {
        if (!menu.visible || !webView) {
            return;
        }

        var menuWidth = menu.width > 0 ? menu.width : menu.implicitWidth;
        var menuHeight = txtMenuEffectiveHeight(menu);

        if (!isTxtMenuHeightReady(menu) && retryCount < txtMenuToolbarTimer.maxRetries) {
            txtMenuToolbarTimer.retryCount = retryCount + 1;
            txtMenuToolbarTimer.start();
            return;
        }

        var webViewPos = webView.mapToItem(null, 0, 0);
        var safeX = toJsInt(menu.x - webViewPos.x);
        var safeY = toJsInt(menu.y - webViewPos.y);
        var safeW = toJsInt(menuWidth);
        var safeH = toJsInt(menuHeight);

        if (safeW <= 0 || safeH <= 0) {
            return;
        }

        webView.runJavaScript(
            "if(typeof setMenuPosition === 'function') setMenuPosition("
            + safeX + ", " + safeY + ", "
            + safeW + ", " + safeH + ");");
    }

    signal deleteNote
    signal moveNote
    signal openSetting
    signal playStateChange(bool state)
    signal saveAudio
    signal saveNote

    // QWebEnginePage::WebAction values are stable across Qt5/Qt6 here:
    // Copy=5, Undo=7, Redo=8.  Keep Tiptap routing in one place so both
    // global shortcuts and context-menu actions target the active editor.
    function triggerTiptapWebAction(action, preferContextTranscript) {
        var tiptapView = tiptapLoader.item ? tiptapLoader.item.editor : null;
        if (!tiptapView) {
            return;
        }

        var webAction = Number(action);
        if (webAction === 7) {
            tiptapView.runJavaScript("window.__dvnTiptapUndo && window.__dvnTiptapUndo()");
            return;
        }
        if (webAction === 8) {
            tiptapView.runJavaScript("window.__dvnTiptapRedo && window.__dvnTiptapRedo()");
            return;
        }
        if (webAction === 5) {
            // 语音转写文本位于 atom NodeView 内部，优先使用专用复制通道；
            // 快捷键只使用当前 DOM 选区/近期选区缓存，避免误用历史右键上下文；
            // 右键菜单 fallback 则允许使用当前 context transcript。
            var transcriptCopyScript = preferContextTranscript
                    ? "window.__dvnTiptapGetContextTranscriptCopyText ? window.__dvnTiptapGetContextTranscriptCopyText() : ''"
                    : "window.__dvnTiptapGetShortcutTranscriptCopyText ? window.__dvnTiptapGetShortcutTranscriptCopyText() : ''";
            tiptapView.runJavaScript(
                transcriptCopyScript,
                function(text) {
                    if (typeof text === "string" && text.length > 0) {
                        TiptapChannel.jsCopyPlainTextToClipboard(text);
                    } else {
                        tiptapView.triggerWebAction(action);
                    }
                });
            return;
        }
        tiptapView.triggerWebAction(action);
    }

    function copy() {
        if (TiptapChannel.tiptapEnabled && tiptapLoader.item) {
            triggerTiptapWebAction(5, false);
        } else {
            webView.triggerWebAction(5);
        }
    }

    Timer {
        id: resourceButtonSyncTimer
        interval: 0
        repeat: false
        onTriggered: rootItem.syncTiptapResourceButtons()
    }

    // Tiptap 工具栏资源按钮由 Web DOM 承载，状态来源却分散在宿主侧
    // 录音、播放、搜索、设备可用性等绑定里。统一走 0ms Timer 合并同步，
    // 让 QML 绑定（尤其 title.recordBtnEnabled 的派生值）先稳定下来，
    // 避免停止录音时先同步到 disabled，后续派生状态变为 enabled 却没有再刷 DOM。
    function scheduleTiptapResourceButtonsSync() {
        if (!TiptapChannel.tiptapEnabled) {
            return;
        }
        resourceButtonSyncTimer.restart();
    }

    // 工具栏中的资源按钮不再由标题栏直接承载，需要把宿主侧运行态
    // 同步到 Tiptap DOM，保持与原 Summernote 入口一致。
    function syncTiptapResourceButtons() {
        if (!TiptapChannel.tiptapEnabled || !tiptapLoader.item) {
            return;
        }

        var tiptapView = tiptapLoader.item.editor;
        if (!tiptapView) {
            return;
        }

        var voiceEnabled = rootItem.recordingAvailable;
        // Summernote 的图片入口原本只受 imageBtnEnable(webVisible) 控制。
        var imageEnabled = webVisible;
        var script = "window.__dvnTiptapSetResourceButtonsEnabled && "
                + "window.__dvnTiptapSetResourceButtonsEnabled("
                + (voiceEnabled ? "true" : "false") + ","
                + (imageEnabled ? "true" : "false") + ");";
        tiptapView.runJavaScript(script);
    }

    onRecordingAvailableChanged: scheduleTiptapResourceButtonsSync()
    onIsRecordingAudioChanged: scheduleTiptapResourceButtonsSync()
    onIsVoiceToTextChanged: scheduleTiptapResourceButtonsSync()
    onWebVisibleChanged: scheduleTiptapResourceButtonsSync()
    onInitialVisibleChanged: scheduleTiptapResourceButtonsSync()

    Component.onCompleted: {
        // changeMode() 可能早于 QML Connections 建立，不能只依赖
        // updateRecordBtnState 信号初始化录音按钮。设备已插入时以当前
        // 可解析的录音设备为准，避免被尚未刷新的缓存误判为不可用。
        title.recorderBtnEnable = VoiceRecoderHandler.isRecordDeviceEnabled();
        scheduleTiptapResourceButtonsSync();
    }

    function focusWebView() {
        // Tiptap 模式下编辑器位于 tiptapLoader 加载的 sourceComponent，需经 tiptapLoader.item
        // 跨 Loader 作用域访问 tiptapWebView；item 为 null（inactive/加载中）时跳过聚焦避免 TypeError。
        // 否则保持原有 Summernote webView 聚焦逻辑。
        if (TiptapChannel.tiptapEnabled) {
            if (tiptapLoader.item) {
                tiptapLoader.item.editor.forceActiveFocus();
                tiptapLoader.item.editor.runJavaScript("window._dvnTiptapFocus && window._dvnTiptapFocus()");
            }
        } else {
            webView.forceActiveFocus();
        }
    }

    function showJsContextMenu() {
        // 仅在编辑区可见时尝试弹出；是否在编辑器内由JS自行判断
        if (webVisible) {
            // 汇总选区类型、必要信息、以及光标/元素坐标
            var js = "(function(){\n" +
                     "  function isInsideEditable(node){\n" +
                     "    var el = node && (node.nodeType === 1 ? node : node.parentElement);\n" +
                     "    return !!(el && el.closest && el.closest('.note-editable'));\n" +
                     "  }\n" +
                     "  var sel = window.getSelection();\n" +
                     "  if(!sel || sel.rangeCount === 0){ return null; }\n" +
                     "  var range = sel.getRangeAt(0);\n" +
                     "  var collapsed = range.collapsed;\n" +
                     "  var inEditable = isInsideEditable(range.commonAncestorContainer || sel.anchorNode);\n" +
                     "  var hasSelection = !collapsed && sel.toString().length > 0;\n" +
                     "  var flags = {\n" +
                     "    canSelectAll: !!inEditable,\n" +
                     "    canCopy: hasSelection,\n" +
                     "    canCut: hasSelection && inEditable,\n" +
                     "    canDelete: hasSelection && inEditable,\n" +
                     "    canPaste: !!inEditable\n" +
                     "  };\n" +
                     "  // 若不在可编辑区内，直接返回空\n" +
                     "  if (!inEditable) { return null; }\n" +
                     "  // 先判断是否为语音/图片选区\n" +
                     "  var kind = 2;\n" +
                     "  var info = '';\n" +
                     "  try {\n" +
                     "    var selected = window.isRangeVoice && window.isRangeVoice();\n" +
                     "    if (selected && typeof selected.flag === 'number') {\n" +
                     "      kind = selected.flag;\n" +
                     "      info = selected.info || '';\n" +
                     "    }\n" +
                     "  } catch(e) {}\n" +
                     "  var rect = null;\n" +
                     "  if (kind === 0) {\n" +
                     "    // 图片：取选区中的第一张图片的矩形\n" +
                     "    var testDiv = (function(){ var s=window.getSelection(); var r=s.getRangeAt(0); var f=r.cloneContents(); var d=document.createElement('div'); d.appendChild(f); return d; })();\n" +
                     "    var img = testDiv.querySelector('img');\n" +
                     "    if (img) rect = img.getBoundingClientRect();\n" +
                     "  } else if (kind === 1) {\n" +
                     "    // 语音：优先取当前激活语音块的位置\n" +
                     "    var voice = document.querySelector('.li.active .voiceBox') || document.querySelector('.voiceBox');\n" +
                     "    if (voice) rect = voice.getBoundingClientRect();\n" +
                     "  }\n" +
                     "  if (!rect) {\n" +
                     "    if (collapsed) {\n" +
                     "      var span = document.createElement('span');\n" +
                     "      span.textContent = '\\u200b';\n" +
                     "      range.insertNode(span);\n" +
                     "      rect = span.getBoundingClientRect();\n" +
                     "      span.parentNode && span.parentNode.removeChild(span);\n" +
                     "    } else {\n" +
                     "      rect = range.getBoundingClientRect();\n" +
                     "    }\n" +
                     "  }\n" +
                     "  if (!rect) return null;\n" +
                     "  return { type: kind, json: info, x: Math.round(rect.left), y: Math.round(rect.bottom), flags: flags };\n" +
                     "})()";
            webView.runJavaScript(js, function(result) {
                if (!result) {
                    // 回退：弹出文本菜单于编辑区中心
                    ActionManager.resetCtxMenu(ActionManager.TxtCtxMenu, true);
                    var fx = Math.max(4, Math.floor(webView.width / 2));
                    var fy = Math.max(4, Math.floor(webView.height / 2));
                    txtCtxMenu.popup(Qt.point(fx, fy));
                    return;
                }
                var px = Math.max(4, Math.min(webView.width - 4, result.x));
                var py = Math.max(4, Math.min(webView.height - 4, result.y));
                if (result.type === 0) {
                    // 图片菜单
                    handler.onSaveMenuParam(0, result.json);
                    picturCtxMenu.popup(Qt.point(px, py));
                    return;
                }
                if (result.type === 1) {
                    // 语音菜单
                    handler.onSaveMenuParam(1, result.json);
                    voiceCtxMenu.popup(Qt.point(px, py));
                    return;
                }
                // 文本菜单：根据 flags 设置可用性
                ActionManager.resetCtxMenu(ActionManager.TxtCtxMenu, false);
                if (result.flags) {
                    ActionManager.enableAction(ActionManager.TxtSelectAll, !!result.flags.canSelectAll);
                    ActionManager.enableAction(ActionManager.TxtCopy, !!result.flags.canCopy);
                    ActionManager.enableAction(ActionManager.TxtCut, !!result.flags.canCut);
                    ActionManager.enableAction(ActionManager.TxtPaste, !!result.flags.canPaste);
                    ActionManager.enableAction(ActionManager.TxtDelete, !!result.flags.canDelete);
                    ActionManager.enableAction(ActionManager.TxtSpeech, !!result.flags.canCopy);
                    ActionManager.enableAction(ActionManager.TxtDictation, !!result.flags.canPaste);
                }
                txtCtxMenu.popup(Qt.point(px, py));
            });
        }
    }

    function startRecording() {
        if (!recordingAvailable) {
            console.log("Cannot start recording: recording is unavailable");
            return false;
        }

        if (!recorderViewLoader.active) {
            recorderViewLoader.active = true;
        } else if (recorderViewLoader.item) {
            recorderViewLoader.item.visible = true;
        }
        isRecording = true;
        title.recorderBtnEnable = false;
        scheduleTiptapResourceButtonsSync();
        return true;
    }

    function resetRecordingUi() {
        isRecording = false;
        title.isRecording = false;
        title.recorderBtnEnable = VoiceRecoderHandler.isRecordDeviceEnabled();
        if (recorderViewLoader.active && recorderViewLoader.item) {
            recorderViewLoader.item.visible = false;
            recorderViewLoader.item.time = "00:00:00";
        }
        Qt.callLater(function() {
            recorderViewLoader.active = false;
            rootItem.scheduleTiptapResourceButtonsSync();
        });
        scheduleTiptapResourceButtonsSync();
    }

    function stopAndClose() {
        recorderViewLoader.item.stop();
    }

    function stopTTS() {
        ActionManager.actionTriggerFromQuick(37);
    }
    
    // 检查当前笔记是否包含录音条目
    function checkHasVoiceContent(callback) {
        if (!webVisible || !webView) {
            callback(false);
            return;
        }
        
        webView.runJavaScript("hasVoice()", function(result) {
            callback(result);
        });
    }

    function toggleMultCho(choices) {
        if (choices > 1) {
            webVisible = false;
            summernoteVisible = false;
            if (!multipleChoicesLoader.active) {
                multipleChoicesLoader.active = true;
            }
            multipleChoicesLoader.visible = true;
            multipleChoicesLoader.item.visible = true;
            multipleChoicesLoader.item.selectSize = choices;
            multipleChoicesLoader.item.setOperationEnabled(!rootItem.isVoiceToText, !rootItem.isVoiceToText);
        } else {
            multipleChoicesLoader.visible = false;
            webVisible = true;
            summernoteVisible = true;
            multipleChoicesLoader.item.visible = false;
        }
    }

    visible: true

    onNoSearchResultChanged: {
        summernoteVisible = !noSearchResult;
    }

    ColumnLayout {
        id: columnLayout

        anchors.fill: parent
        spacing: 0

        Item {
            id: titleBarHost

            Layout.fillWidth: true
            Layout.minimumHeight: 50
            Layout.maximumHeight: 50

            // 透明窗口下仅标题栏区域遮挡毛玻璃，沿用窗口 palette 底色（非 TitleBar 强制白底）
            Rectangle {
                anchors.fill: parent
                color: Window.window ? Window.window.palette.window
                                      : (DTK.themeType === ApplicationHelper.LightType ? "#FFFFFF" : "#242424")
                z: -1
            }

            WindowTitleBar {
                Accessible.name: "WebViewTitleBar"
                Accessible.role: Accessible.Pane

                id: title

                anchors.fill: parent
                isInitialInterface: initialVisible
                isRecordingAudio: rootItem.isRecordingAudio
                isVoiceToText: rootItem.isVoiceToText

                onTitleOpenSetting: {
                    rootItem.openSetting();
                }
            }
        }

        Rectangle {
            id: noSearchRect

            Layout.fillHeight: true
            Layout.fillWidth: true
            // 设置基础背景颜色，和 web 前端共同实现背景色
            color: webView.backgroundColor
            visible: noSearchResult

            DciIcon {
                id: noSearchIcon

                anchors.centerIn: noSearchRect
                name: "search_no_results"
                theme: DTK.themeType
                visible: noSearchResult
            }
        }

        Rectangle {
            id: webRect

            Layout.fillHeight: true
            Layout.fillWidth: true
            visible: summernoteVisible && !TiptapChannel.tiptapEnabled
            color: DTK.themeType === ApplicationHelper.LightType ? "#FFFFFF" : "#242424"

            WebEngineView {
                Accessible.ignored: true

                id: webView

                anchors.fill: parent
                // 设置基础背景颜色，和 web 前端共同实现背景色
                backgroundColor: DTK.themeType === ApplicationHelper.LightType ? "white" : "black"
                visible: webVisible

                Component.onCompleted: {
                    if (!TiptapChannel.tiptapEnabled) {
                        noteWebChannel.registerObject("webobj", Webobj);
                        // console.log("registerObject ret: " + ret)
                        webView.webChannel = noteWebChannel;
                        webView.url = Qt.resolvedUrl(Webobj.webPath());

                        // 隐藏浮动工具栏
                        Webobj.calllJsShowEditToolbar(0, 0);
                    }
                }
                onContextMenuRequested: req => {
                    // 初始页可见或编辑器不可见时，屏蔽右键菜单
                    if (initialVisible || !webVisible) {
                        req.accepted = true;
                        return;
                    }
                    // 仅当菜单是文本类型时，我们才在QML层根据上下文标记(editFlags)更新状态
                    // 这是为了精确修复Qt5下文本菜单状态不更新的问题
                    if (handler.menuTypeFromJs === WebEngineHandler.TxtMenu) {
                        var flags = req.editFlags
                        ActionManager.resetCtxMenu(ActionManager.TxtCtxMenu, false) // false表示禁用全部，然后逐一启用
                        ActionManager.enableAction(ActionManager.TxtSelectAll, (flags & 1) !== 0)
                        ActionManager.enableAction(ActionManager.TxtCopy, (flags & 2) !== 0)
                        ActionManager.enableAction(ActionManager.TxtCut, (flags & 4) !== 0)
                        ActionManager.enableAction(ActionManager.TxtPaste, (flags & 8) !== 0)
                        ActionManager.enableAction(ActionManager.TxtDelete, (flags & 16) !== 0)
                        // 只有能复制时，才能朗读
                        ActionManager.enableAction(ActionManager.TxtSpeech, (flags & 2) !== 0)
                        // 只有能粘贴时，才能听写
                        ActionManager.enableAction(ActionManager.TxtDictation, (flags & 8) !== 0)
                    }

                    // 对于所有菜单类型，都调用C++的处理器
                    // C++将处理特殊逻辑(如解析语音路径)并最终弹出菜单
                    handler.onContextMenuRequested(req)

                    // 阻止默认菜单弹出
                    req.accepted = true;
                }
                onJavaScriptConsoleMessage: {
                    // 调试使用，打印控制台输出
                    console.debug("--- from web: ", message, sourceID, lineNumber);
                }

                DropArea {
                    anchors.fill: parent

                    property bool currentDragCanDropImages: false

                    onEntered: function(drag) {
                        currentDragCanDropImages = drag.hasUrls && VNoteMainManager.canInsertImages(drag.urls);
                        drag.accepted = currentDragCanDropImages;
                    }
                    onExited: {
                        currentDragCanDropImages = false;
                    }
                    onPositionChanged: function(drag) {
                        drag.accepted = currentDragCanDropImages;
                    }
                    onDropped: function(drop) {
                        if (currentDragCanDropImages) {
                            drop.accepted = true;
                            VNoteMainManager.insertImages(drop.urls);
                        } else {
                            drop.accepted = false;
                        }
                        currentDragCanDropImages = false;
                    }
                }

                WebEngineHandler {
                    id: handler

                    target: webView

                    onLoadRichText: {
                        VNoteMainManager.vNoteChanged(itemListView.model.get(0).noteId);
                        itemListView.selectedNoteItem = [0];
                        itemListView.selectSize = 1;
                    }
                    onPlayingVoice: isPlay => {
                        playStateChange(isPlay);
                        title.isPlaying = isPlay;
                        rootItem.scheduleTiptapResourceButtonsSync();
                    }
                    onPopupToast: (message, msgId) => {
                        DTK.sendMessage(webView, message, "icon_warning", 4000, msgId);
                    }
                    onRequesetCallJsSynchronous: func => {
                        webView.runJavaScript(func, function (result) {
                            onCallJsResult(result);
                        });
                    }
                    onRequestMessageDialog: type => {
                        // 触发创建提示对话框
                        if (type === VNoteMessageDialogHandler.UpdateUosAi) {
                            messageDialogLoader.showDialog(type, ret => {
                                if (ret)
                                    VNoteMainManager.openUosAiInAppStore();
                            });
                        } else {
                            messageDialogLoader.showDialog(type);
                        }
                    }
                    onTriggerWebAction: action => {
                        if (TiptapChannel.tiptapEnabled && tiptapLoader.item) {
                            triggerTiptapWebAction(action, true);
                        } else {
                            webView.triggerWebAction(action);
                        }
                    }
                    onViewPicture: filePath => {
                        viewPictureLoader.path = filePath;
                        if (!viewPictureLoader.active)
                            viewPictureLoader.active = true;
                        else
                            viewPictureLoader.item.show();
                    }
                    onSaveAudio: {
                        rootItem.saveAudio();
                    }
                    onCreateNote: {
                        if (!initialVisible && !rootItem.isVoiceToText) {
                            VNoteMainManager.createNote();
                        }
                    }
                }

                WebChannel {
                    id: noteWebChannel

                }
            }
        }


        // Tiptap 默认富文本 WebEngineView
        Loader {
            id: tiptapLoader

            active: TiptapChannel.tiptapEnabled
            // 搜索无结果时由 noSearchRect 占据编辑区，Tiptap 不应继续参与布局，
            // 否则会被 ColumnLayout 排到无结果区域下方，导致工具栏出现在底部。
            visible: active && !noSearchResult
            Layout.fillHeight: true
            Layout.fillWidth: true
            onWidthChanged: rootItem.scheduleTiptapToolbarSeparatorSync()
            onHeightChanged: rootItem.scheduleTiptapToolbarSeparatorSync()
            onYChanged: rootItem.scheduleTiptapToolbarSeparatorSync()

            sourceComponent: Item {
                Accessible.name: "TiptapWebView"
                Accessible.role: Accessible.Pane
                property WebEngineView editor: tiptapWebView
                anchors.fill: parent

                WebEngineView {
                    Accessible.ignored: true

                    id: tiptapWebView

                    property bool dvnResettingNativeZoom: false

                    anchors.fill: parent
                    backgroundColor: DTK.themeType === ApplicationHelper.LightType ? "white" : "black"
                    visible: true

                    settings.localContentCanAccessFileUrls: true

                    Component.onCompleted: {
                        tiptapWebChannel.registerObject("tiptapChannel", TiptapChannel);
                        tiptapWebView.webChannel = tiptapWebChannel;
                        tiptapWebView.url = Qt.resolvedUrl(TiptapChannel.tiptapHtmlPath());
                    }

                    onLoadingChanged: function(loadRequest) {
                        if (loadRequest.status === WebEngineView.LoadSucceededStatus) {
                            if (Math.abs(tiptapWebView.zoomFactor - 1.0) >= 0.001) {
                                tiptapWebView.dvnResettingNativeZoom = true;
                                tiptapWebView.zoomFactor = 1.0;
                                Qt.callLater(function() { tiptapWebView.dvnResettingNativeZoom = false; });
                            }
                            tiptapWebView.forceActiveFocus();
                            tiptapWebView.runJavaScript("window._dvnTiptapFocus && window._dvnTiptapFocus()");
                            handler.onThemeChanged();
                            rootItem.scheduleTiptapResourceButtonsSync();
                            rootItem.scheduleTiptapToolbarSeparatorSync();
                        }
                    }

                    onWidthChanged: rootItem.scheduleTiptapToolbarSeparatorSync()
                    onHeightChanged: rootItem.scheduleTiptapToolbarSeparatorSync()
                    onZoomFactorChanged: {
                        if (tiptapWebView.dvnResettingNativeZoom) {
                            rootItem.scheduleTiptapToolbarSeparatorSync();
                        } else {
                            rootItem.consumeTiptapNativeZoomFactor(tiptapWebView);
                        }
                    }

                    onContextMenuRequested: req => {
                        req.accepted = true;
                        var rawX = Number(req.position.x);
                        var rawY = Number(req.position.y);
                        if (isNaN(rawX) || isNaN(rawY)) return;
                        var sx = String(Math.round(rawX));
                        var sy = String(Math.round(rawY));
                        // Qt WebEngine 已经根据系统剪贴板计算了 CanPaste；
                        // Tiptap 文本菜单不能仅以“位于编辑器内”判定可粘贴。
                        var nativeEditFlags = Number(req.editFlags);
                        tiptapWebView.runJavaScript(
                            "(function(){"
                            + "function flags(canSelectAll,canCopy,canCut,canPaste,canDelete){return {canSelectAll:!!canSelectAll,canCopy:!!canCopy,canCut:!!canCut,canPaste:!!canPaste,canDelete:!!canDelete,canSpeech:!!canCopy,canDictation:!!canPaste};}"
                            + "function selectionTouches(target){var sel=window.getSelection();if(!sel||sel.rangeCount===0||sel.isCollapsed||!sel.toString())return false;for(var i=0;i<sel.rangeCount;i++){var r=sel.getRangeAt(i);if(r.intersectsNode){try{if(r.intersectsNode(target))return true;}catch(e){}}else{var cr=document.createRange();cr.selectNodeContents(target);if(r.compareBoundaryPoints(Range.END_TO_START,cr)>0&&r.compareBoundaryPoints(Range.START_TO_END,cr)<0)return true;}}return false;}"
                            + "var el=document.elementFromPoint(" + sx + "," + sy + ");"
                            + "var capturedTranscript=(window.__dvnTiptapContextTranscript&&document.contains(window.__dvnTiptapContextTranscript))?window.__dvnTiptapContextTranscript:null;"
                            + "if(!el&&capturedTranscript){var copyText0=window.__dvnTiptapGetContextTranscriptCopyText?window.__dvnTiptapGetContextTranscriptCopyText():'';return JSON.stringify({type:2,json:'__tiptap_transcript_text:'+copyText0,flags:flags(true,!!copyText0,false,false,false),transcript:true});}"
                            + "if(!el){window.__dvnTiptapContextTranscript=null;return JSON.stringify({type:2,json:'',flags:flags(true,false,false,true,false)});}"
                            + "var transcript=el.closest?el.closest('.translateText'):null;"
                            + "if(!transcript) transcript=capturedTranscript;"
                            + "if(transcript){window.__dvnTiptapContextTranscript=transcript;var copyText=window.__dvnTiptapGetContextTranscriptCopyText?window.__dvnTiptapGetContextTranscriptCopyText():'';var canCopy=selectionTouches(transcript)||!!copyText;return JSON.stringify({type:2,json:'__tiptap_transcript_text:'+copyText,flags:flags(true,canCopy,false,false,false),transcript:true});}"
                            + "window.__dvnTiptapContextTranscript=null;"
                            + "var vb=el.closest?el.closest('.voiceInfoBox'):null;"
                            + "if(vb&&vb.getAttribute('data-type')==='voice-block'){if(window.__dvnTiptapSelectVoiceBlockFromElement)window.__dvnTiptapSelectVoiceBlockFromElement(vb);return JSON.stringify({type:1,json:vb.getAttribute('data-voice-meta')||''});}"
                            + "var img=el.closest?el.closest('img[data-rel-path]'):null;"
                            + "if(img){if(window.__dvnTiptapSelectImageFromElement)window.__dvnTiptapSelectImageFromElement(img);return JSON.stringify({type:0,json:img.getAttribute('src')||img.getAttribute('data-rel-path')||''});}"
                            + "var sel=window.getSelection();var hasSelection=!!(sel&&sel.rangeCount>0&&!sel.isCollapsed&&sel.toString());var inEditor=!!(el.closest&&el.closest('.ProseMirror'));"
                            + "return JSON.stringify({type:2,json:'',flags:flags(inEditor,hasSelection,hasSelection&&inEditor,inEditor,hasSelection&&inEditor)});"
                            + "})()",
                            function(result) {
                                var info = null;
                                try { info = JSON.parse(result); } catch(e) {}
                                if (!info) return;
                                if (info.type === 0) {
                                    handler.onSaveMenuParam(info.type, info.json);
                                    handler.onContextMenuRequested(req);
                                    return;
                                }
                                if (info.type === 2) {
                                    handler.onSaveMenuParam(info.type, info.json || "");
                                    ActionManager.resetCtxMenu(ActionManager.TxtCtxMenu, false);
                                    ActionManager.visibleAction(ActionManager.TxtStopreading, false);
                                    var flags = info.flags || {};
                                    // 普通文本菜单使用 Qt 的实际剪贴板状态；
                                    // 转写文本仍保持只读语义，只允许选择/复制。
                                    if (!info.transcript && Number.isFinite(nativeEditFlags)) {
                                        flags.canPaste = (nativeEditFlags & 8) !== 0;
                                        flags.canDictation = flags.canPaste;
                                    }
                                    ActionManager.enableAction(ActionManager.TxtSelectAll, !!flags.canSelectAll);
                                    ActionManager.enableAction(ActionManager.TxtCopy, !!flags.canCopy);
                                    ActionManager.enableAction(ActionManager.TxtCut, !!flags.canCut);
                                    ActionManager.enableAction(ActionManager.TxtPaste, !!flags.canPaste);
                                    ActionManager.enableAction(ActionManager.TxtDelete, !!flags.canDelete);
                                    ActionManager.enableAction(ActionManager.TxtSpeech, !!flags.canSpeech);
                                    ActionManager.enableAction(ActionManager.TxtDictation, !!flags.canDictation);
                                    txtCtxMenu.popup(Qt.point(rawX, rawY));
                                    return;
                                }
                                handler.onSaveMenuParam(info.type, info.json);
                                handler.onContextMenuRequested(req);
                            });
                    }
                }

                DropArea {
                    anchors.fill: parent

                    property bool currentDragCanDropImages: false

                    onEntered: function(drag) {
                        currentDragCanDropImages = drag.hasUrls && VNoteMainManager.canInsertImages(drag.urls);
                        drag.accepted = currentDragCanDropImages;
                    }
                    onExited: {
                        currentDragCanDropImages = false;
                    }
                    onPositionChanged: function(drag) {
                        drag.accepted = currentDragCanDropImages;
                    }
                    onDropped: function(drop) {
                        if (currentDragCanDropImages) {
                            drop.accepted = true;
                            rootItem.insertImagesAtTiptapClientPoint(drop.urls, drop.x, drop.y);
                        } else {
                            drop.accepted = false;
                        }
                        currentDragCanDropImages = false;
                    }
                }

                WebChannel {
                    id: tiptapWebChannel
                }
            }
        }

        Loader {
            id: multipleChoicesLoader

            Layout.fillHeight: parent.height
            Layout.fillWidth: parent.width
            visible: false

            sourceComponent: MultipleChoices {
                Accessible.name: "MultipleChoicesView"
                Accessible.role: Accessible.Pane

                anchors.fill: parent

                onDeleteNote: {
                    rootItem.deleteNote();
                }
                onMoveNote: {
                    rootItem.moveNote();
                }
                onSaveAudio: {
                    rootItem.saveAudio();
                }
                onSaveNote: {
                    rootItem.saveNote();
                }
            }
        }
    }

    Item {
        id: toolbarSeparatorOverlay

        // QtWebEngine 的滚动条槽不属于 DOM 内容绘制区，Web 里的 border/伪元素
        // 不能可靠覆盖最右侧滚动条区域。工具栏上下分割线统一由 QML 宿主
        // 叠加绘制，宽度直接覆盖整个富文本容器，避免右侧断线。
        anchors.left: parent.left
        anchors.right: parent.right
        height: Math.max(toolbarBottomY, toolbarTopY)
        visible: webVisible && TiptapChannel.tiptapEnabled && !noSearchResult
        z: 2000

        readonly property real defaultToolbarHeight: 48
        property real toolbarTopY: titleBarHost.height
        property real toolbarBottomY: titleBarHost.height + defaultToolbarHeight
        readonly property color separatorColor: DTK.themeType === ApplicationHelper.LightType ? "#14000000" : "#1FFFFFFF"

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            y: toolbarSeparatorOverlay.toolbarTopY
            color: toolbarSeparatorOverlay.separatorColor
            height: 1 / Screen.devicePixelRatio
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            y: toolbarSeparatorOverlay.toolbarBottomY - height
            color: toolbarSeparatorOverlay.separatorColor
            height: 1 / Screen.devicePixelRatio
        }
    }

    VNoteMessageDialogLoader {
        Accessible.name: "WebViewMessageDialog"
        Accessible.role: Accessible.Dialog

        id: messageDialogLoader

    }

    Loader {
        id: viewPictureLoader

        property string path: ""

        asynchronous: true

        sourceComponent: ViewPictureDialog {
            Accessible.name: "ViewPictureDialog"
            Accessible.role: Accessible.Dialog

            id: viewPictureWindow

            filePath: path
        }

        onActiveChanged: {
            if (active)
                viewPictureWindow.show();
        }
    }

    Loader {
        asynchronous: true

        VNoteRightMenu {
            Accessible.name: "PictureContextMenu"
            Accessible.role: Accessible.PopupMenu

            id: picturCtxMenu

            menuType: ActionManager.PictureCtxMenu

            onAboutToShow: {
                // 对于图片菜单，我们恢复原始逻辑：在菜单显示前，重置所有项目为可用
                // C++后端会处理图片路径等特殊逻辑
                ActionManager.resetCtxMenu(ActionManager.PictureCtxMenu, true);
            }

            Connections {
                target: handler

                onRequestShowMenu: (type, pos) => {
                    if (type === WebEngineHandler.PictureMenu) {
                        picturCtxMenu.popup(pos);
                    }
                }
            }
        }
    }

    Loader {
        asynchronous: true

        VNoteRightMenu {
            Accessible.name: "VoiceContextMenu"
            Accessible.role: Accessible.PopupMenu

            id: voiceCtxMenu

            menuType: ActionManager.VoiceCtxMenu

            onAboutToShow: {
                // 对于语音菜单，恢复原始逻辑：重置所有项目为可用，
                // 并根据全局状态单独设置"语音转文字"的可用性
                ActionManager.resetCtxMenu(ActionManager.VoiceCtxMenu, true);
                var isConverting = VNoteMainManager.isVoiceToText();
                ActionManager.enableAction(ActionManager.VoiceToText, !isConverting);
                // 播放中禁用删除
                ActionManager.enableAction(ActionManager.VoiceDelete, !titleBar.isPlaying);
            }

            Connections {
                target: handler

                onRequestShowMenu: (type, pos) => {
                    if (type === WebEngineHandler.VoiceMenu) {
                        voiceCtxMenu.popup(pos);
                    }
                }
            }
        }
    }

    Loader {
        asynchronous: true

        VNoteRightMenu {
            Accessible.name: "TextContextMenu"
            Accessible.role: Accessible.PopupMenu

            id: txtCtxMenu

            menuType: ActionManager.TxtCtxMenu

            onAboutToShow: {
                // 文本菜单的状态现在完全由 onContextMenuRequested 处理，
                // 此处不再需要任何逻辑。
            }

            onOpened: {
                txtMenuToolbarTimer.stop();
                txtMenuToolbarTimer.retryCount = 0;
                Qt.callLater(function() {
                    rootItem.passTxtMenuToToolbar(txtCtxMenu, 0);
                });
            }

            onClosed: {
                txtMenuToolbarTimer.stop();
                // 菜单关闭不隐藏工具栏，仅解除 Summernote 定位锁定（b80965d 原始行为）
                if (!TiptapChannel.tiptapEnabled) {
                    webView.runJavaScript(
                        "if(typeof restoreAirPopoverTooltipPlacement === 'function') restoreAirPopoverTooltipPlacement();");
                }
            }

            Connections {
                target: handler

                onRequestShowMenu: (type, pos) => {
                    if (type === WebEngineHandler.TxtMenu) {
                        txtCtxMenu.popup(pos);
                    }
                }
            }
        }
    }

    Loader {
        id: selectImgLoader

        active: false
        asynchronous: true

        sourceComponent: FileDialog {
            id: fileDialog

            fileMode: FileDialog.OpenFiles
            folder: StandardPaths.writableLocation(StandardPaths.PicturesLocation)
            nameFilters: ["Image file(*.jpg *.png *.bmp)"]

            Component.onCompleted: {
                fileDialog.open();
            }
            onAccepted: {
                if (fileDialog.files.length > 0) {
                    VNoteMainManager.insertImages(fileDialog.files);
                } else {
                    rootItem.clearTiptapInsertionSelection();
                }
            }
            onRejected: {
                rootItem.clearTiptapInsertionSelection();
            }
        }
    }

    Loader {
        id: recorderViewLoader
            z: 1000

        function onPauseRecording() {
            VoiceRecoderHandler.pauseRecoder();
        }

        function onStopRecording() {
            VoiceRecoderHandler.stopRecoder();
        }

        active: false
        anchors.bottom: rootItem.bottom
        anchors.horizontalCenter: rootItem.horizontalCenter
        asynchronous: true
        height: 70
        width: rootItem.width

        sourceComponent: RecordingView {
            Accessible.name: "RecordingBar"
            Accessible.role: Accessible.Pane

            id: recordingBar

            anchors.fill: parent

            Component.onCompleted: {
                recordingBar.visible = true;
            }
        }

        onLoaded: {
            recorderViewLoader.item.pauseRecording.connect(onPauseRecording);
            recorderViewLoader.item.stopRecording.connect(onStopRecording);
            recorderViewLoader.item.forceActiveFocus();
        }
    }

    Connections {
        target: title

        onIsPlayingChanged: rootItem.scheduleTiptapResourceButtonsSync()
        onIsSearchingChanged: rootItem.scheduleTiptapResourceButtonsSync()
        onIsRecordingAudioChanged: rootItem.scheduleTiptapResourceButtonsSync()
        onIsVoiceToTextChanged: rootItem.scheduleTiptapResourceButtonsSync()
        onRecorderBtnEnableChanged: rootItem.scheduleTiptapResourceButtonsSync()
        onRecordBtnEnabledChanged: rootItem.scheduleTiptapResourceButtonsSync()
    }

    Connections {
        target: TiptapChannel

        onVoicePlaybackStateChanged: function(voiceId, state) {
            // 与 Summernote 的 WebEngineHandler::onPlayingVoice 保持一致：
            // 播放和暂停都属于“正在占用播放状态”，只有结束才恢复录音。
            title.isPlaying = (state !== 2);
            rootItem.scheduleTiptapResourceButtonsSync();
        }

        onPickImageRequested: {
            if (!webVisible || VNoteMainManager.isInSearchMode()) {
                rootItem.clearTiptapInsertionSelection();
                return;
            }
            if (!selectImgLoader.active) {
                selectImgLoader.active = true;
            } else {
                selectImgLoader.item.open();
            }
        }
        onRecordVoiceRequested: {
            if (VNoteMainManager.isInSearchMode() || !title.recordBtnEnabled || !webVisible) {
                return;
            }
            if (startRecording()) {
                VoiceRecoderHandler.startRecoder();
            }
        }
        onViewPictureRequested: path => {
            viewPictureLoader.path = path;
            if (!viewPictureLoader.active)
                viewPictureLoader.active = true;
            else
                viewPictureLoader.item.show();
        }
    }

    Connections {
        target: VNoteMainManager

        onNeedUpdateNote: function(noteId) {
            var requestNoteId = noteId;
            if (TiptapChannel.tiptapEnabled) {
                TiptapChannel.requestEditorContent();
            } else {
                webView.runJavaScript("getHtml()", function (result) {
                    VNoteMainManager.updateNoteWithResultForNote(requestNoteId, result);
                });
            }
        }
        onScrollChange: isTop => {
            hasScroll = !isTop;
        }
        onUpdateRichTextSearch: key => {
            if (TiptapChannel.tiptapEnabled) {
                if (key && key.length > 0) {
                    TiptapChannel.setSearchQuery(key);
                } else {
                    TiptapChannel.clearSearchQuery();
                }
            } else {
                webView.findText(key);
            }
        }
    }

    Connections {
        target: VoiceRecoderHandler

        onRecoderStateChange: function(type) {
            var currentType = type;
            if (recorderViewLoader.item) {
                recorderViewLoader.item.isRecording = (currentType === VoiceRecoderHandler.Recording);
            }

            // 当录音状态变为Idle时，完全关闭录音界面并重置状态
            if (currentType === VoiceRecoderHandler.Idle) {
                // 停止、启动失败以及设备热插拔都统一走同一套 UI 恢复逻辑。
                rootItem.resetRecordingUi();
            }
            rootItem.scheduleTiptapResourceButtonsSync();
        }
        onUpdateRecordBtnState: function(enable) {
            title.recorderBtnEnable = enable;
            rootItem.scheduleTiptapResourceButtonsSync();
        }
        onUpdateRecorderTime: function(time) {
            if (recorderViewLoader.item) {
                recorderViewLoader.item.time = time;
            }
        }
        onVolumeTooLow: isLow => {
            if (isLow) {
                messageDialogLoader.showDialog(VNoteMessageDialogHandler.VolumeTooLow, ret => {
                    if (ret) {
                        // 录音界面在低音量确认前已经展示，确认时只启动后端，
                        // 不要再次创建/定位录音界面。
                        VoiceRecoderHandler.confirmStartRecoder();
                    } else {
                        rootItem.resetRecordingUi();
                    }
                });
            }
        }
    }

    Connections {
        target: VNoteMainManager

        onSaveVoiceStateChanged: enabled => {
            if (multipleChoicesLoader.active && multipleChoicesLoader.item) {
                multipleChoicesLoader.item.setSaveVoiceEnabled(enabled);
            }
        }
        onVoiceToTextStateChanged: isConverting => {
            if (multipleChoicesLoader.active && multipleChoicesLoader.item) {
                multipleChoicesLoader.item.setOperationEnabled(!isConverting, !isConverting);
            }
        }
    }

    Connections {
        target: Webobj

        onCallJsSelectAll: {
            if (TiptapChannel.tiptapEnabled && tiptapLoader.item) {
                tiptapWebView.runJavaScript(
                    "if(window.__dvnTiptapSelectContextAll)window.__dvnTiptapSelectContextAll();" +
                    "else if(window.__dvnTiptapEditor)window.__dvnTiptapEditor.chain().selectAll().run();");
            }
        }
        onCallJsDeleteSelection: {
            if (TiptapChannel.tiptapEnabled && tiptapLoader.item) {
                tiptapWebView.runJavaScript(
                    "if(window.__dvnTiptapEditor)window.__dvnTiptapEditor.chain().deleteSelection().run()");
            }
        }
        onCallJsFocusEditor: {
            if (TiptapChannel.tiptapEnabled && tiptapLoader.item) {
                tiptapWebView.runJavaScript(
                    "if(window.__dvnTiptapEditor)window.__dvnTiptapEditor.commands.focus()");
            }
        }
    }
}
