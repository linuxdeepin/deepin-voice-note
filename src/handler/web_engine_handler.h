// SPDX-FileCopyrightText: 2024-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef WEBENGINEHANDLER_H
#define WEBENGINEHANDLER_H

#include "actionmanager.h"

#include <QVariant>
#include <QObject>
#include <QRect>
#include <QEventLoop>
#include <QUrl>
#include <QVariantMap>

#include <QTimer>

#include "vnote_message_dialog_handler.h"

class QDBusInterface;

class VNVoiceBlock;
class VoicePlayerHandler;
class VoiceToTextHandler;

class WebEngineHandler : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject *target READ target WRITE setTarget NOTIFY targetChanged FINAL)
    // QML 侧用 handler.menuTypeFromJs 判断当前菜单类型（WebEngineView.qml 右键菜单状态刷新）；
    // 上游仅有成员变量未暴露属性，这里补齐暴露（只增不改，不改原有语义）
    Q_PROPERTY(Menu menuTypeFromJs READ menuTypeFromJs NOTIFY menuTypeChanged FINAL)

public:
    enum Menu {
        PictureMenu = 0,
        VoiceMenu,
        TxtMenu,
        MaxMenu,
    };
    Q_ENUM(Menu)

    explicit WebEngineHandler(QObject *parent = nullptr);

    QObject *target() const;
    void setTarget(QObject *targetWebEngine);
    Q_SIGNAL void targetChanged();

    QVariant callJsSynchronous(const QString &function);

    Menu menuTypeFromJs() const { return menuType; }

Q_SIGNALS:
    // 请求右键菜单
    void requestShowMenu(Menu type, const QPoint &pos);
    // 请求同步调用Js函数
    void requesetCallJsSynchronous(const QString &func);
    // 触发 web 常用文本操作
    // 枚举传递到 QML 会变成 undefined，统一使用 int：
    //   Cut=4 Copy=5 Paste=6 Undo=7 Redo=8 SelectAll=9
    // （与 QWebEnginePage::WebAction 稳定值保持一致）
    void triggerWebAction(int action);

    void menuTypeChanged();

    // 请求弹出提示对话框
    void requestMessageDialog(VNoteMessageDialogHandler::MessageType type);

    void loadRichText();

    void viewPicture(const QString &filePath);

    void popupToast(const QString &message, int msgId);

    void playingVoice(bool isPlay);

    void saveAudio();
    void createNote();

public Q_SLOTS:
    // for qml
    void onCallJsResult(const QVariant &result);
    // 引擎无关的右键菜单请求，payload 见 WebKitWebView::contextMenuRequested：
    //   position(QPointF) / editFlags(int) / x / y (int)
    void onContextMenuRequested(const QVariantMap &request);
    void onInsertVoiceItem(const QString &voicePath, quint64 voiceSize);

    // for c++
    void onThemeChanged();
    void onSaveMenuParam(int type, const QVariant &json);
    void onMenuClicked(ActionManager::ActionKind kind);
    void onPaste(bool isVoice);

private:
    void initFontsInformation();
    void connectWebContent();

    void processVoiceMenuRequest(const QVariantMap &request);
    void processTextMenuRequest(const QVariantMap &request);
    void processPictureMenuRequest(const QVariantMap &request);

    bool isVoicePaste();

    bool saveMP3();
    void savePictureAs();
    QString saveAsFile(const QString &originalPath, QString dirPath, const QString &defalutName);

private:
    QObject *m_targetWebEngine { nullptr };  // 关联的 qml web 视图

    VoicePlayerHandler *m_voicePlayerHandler { nullptr };  // 语音播放管理
    VoiceToTextHandler *m_voiceToTextHandler { nullptr };  // 语音转文字管理

    QRect m_editToolbarRect;

    Menu menuType { MaxMenu };  // 菜单类型
    QVariant menuJson;          // 菜单数据

    QSharedPointer<VNVoiceBlock> m_voiceBlock { nullptr };  // 待另存的语音数据

    // 音频服务dbus接口 (org.deepin.dde.Audio)
    QDBusInterface *m_appearanceDBusInterface { nullptr };

    QStringList m_fontList;  // 字体列表
    QString m_defaultFont;   // 默认字体

    QVariant m_callJsResult;  // js 函数调用返回值
    QEventLoop m_callJsLoop;  // js 函数调用等待
};

#endif  // WEBENGINEHANDLER_H
