// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef WEBENGINEHANDLER_H
#define WEBENGINEHANDLER_H

#include "actionmanager.h"

#include <QVariant>
#include <QObject>
#include <QRect>
#include <QEventLoop>
#include <QWebEnginePage>
#include <QTimer>

#include "vnote_message_dialog_handler.h"

class QDBusInterface;
class QWebEngineContextMenuRequest;

class VNVoiceBlock;
class VoicePlayerHandler;
class VoiceToTextHandler;

class WebEngineHandler : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject *target READ target WRITE setTarget NOTIFY targetChanged FINAL)

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

Q_SIGNALS:
    // 请求右键菜单
    void requestShowMenu(Menu type, const QPoint &pos);
    // 请求同步调用Js函数
    void requesetCallJsSynchronous(const QString &func);
    // 触发 web 常用文本操作
    void triggerWebAction(QWebEnginePage::WebAction action);

    // 请求弹出提示对话框
    void requestMessageDialog(VNoteMessageDialogHandler::MessageType type);

    void loadRichText();

    void viewPicture(const QString &filePath);

public Q_SLOTS:
    // for qml
    void onCallJsResult(const QVariant &result);
    void onContextMenuRequested(QWebEngineContextMenuRequest *request);
    void onInsertVoiceItem(const QString &voicePath, quint64 voiceSize);

    // for c++
    void onThemeChanged();
    void onSaveMenuParam(int type, const QVariant &json);
    void onMenuClicked(ActionManager::ActionKind kind);
    void onPaste(bool isVoice);

private:
    void initFontsInformation();
    void connectWebContent();

    void processVoiceMenuRequest(QWebEngineContextMenuRequest *request);
    void processTextMenuRequest(QWebEngineContextMenuRequest *request);

    bool isVoicePaste();

    bool saveMP3();
    void savePictureAs();
    QString saveAsFile(const QString &originalPath, QString dirPath, const QString &defalutName);

private:
    QObject *m_targetWebEngine { nullptr };  // 关联的 qml WebEngineView

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
