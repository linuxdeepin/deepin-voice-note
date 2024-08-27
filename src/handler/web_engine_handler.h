// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef WEBENGINEHANDLER_H
#define WEBENGINEHANDLER_H

#include "actionmanager.h"

#include <QVariant>
#include <QObject>
#include <QRect>

class QDBusInterface;
class QWebEngineContextMenuRequest;

class VNVoiceBlock;
class VoicePlayerHandler;

class WebEngineHandler : public QObject
{
    Q_OBJECT
public:
    enum Menu {
        PictureMenu = 0,
        VoiceMenu,
        TxtMenu,
        MaxMenu,
    };
    Q_ENUM(Menu)

    explicit WebEngineHandler(QObject *parent = nullptr);

Q_SIGNALS:
    // 请求右键菜单
    void requestShowMenu(Menu type, const QPoint &pos);
    // 开始语音转文字
    void asrStart(const QSharedPointer<VNVoiceBlock> voiceBlock);

public Q_SLOTS:
    // for qml
    void onContextMenuRequested(QWebEngineContextMenuRequest *request);

    void onInsertVoiceItem(const QString &voicePath, quint64 voiceSize);

    // for c++
    void onThemeChanged();
    void onSaveMenuParam(int type, const QVariant &json);

    void onMenuClicked(ActionManager::ActionKind kind);

private:
    void initFontsInformation();
    void connectWebContent();

    void processVoiceMenuRequest(QWebEngineContextMenuRequest *request);
    void processTextMenuRequest(QWebEngineContextMenuRequest *request);

private:
    VoicePlayerHandler *voicePlayerHandler { nullptr };

    QRect m_editToolbarRect;

    Menu menuType { MaxMenu };  // 菜单类型
    QVariant menuJson;          // 菜单数据

    QSharedPointer<VNVoiceBlock> voiceBlock { nullptr };  // 待另存的语音数据

    // 音频服务dbus接口 (org.deepin.dde.Audio)
    QDBusInterface *m_appearanceDBusInterface { nullptr };

    QStringList m_fontList;  // 字体列表
    QString m_defaultFont;   // 默认字体
};

#endif  // WEBENGINEHANDLER_H
