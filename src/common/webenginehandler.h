// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef WEBENGINEHANDLER_H
#define WEBENGINEHANDLER_H

#include <QVariant>
#include <QObject>
#include <QRect>

class QDBusInterface;
class QWebEngineContextMenuRequest;

class VNVoiceBlock;

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
    void requestShowMenu(Menu type, const QPoint &pos);

public Q_SLOTS:
    // for qml
    void onContextMenuRequested(QWebEngineContextMenuRequest *request);

    // for c++
    void onThemeChanged();
    void onSaveMenuParam(int type, const QVariant &json);

private:
    void initFontsInformation();
    void connectWebContent();

    void processVoiceMenuRequest(QWebEngineContextMenuRequest *request);
    void processTextMenuRequest(QWebEngineContextMenuRequest *request);

private:
    QRect m_editToolbarRect;

    Menu menuType { MaxMenu };   // 菜单类型
    QVariant menuJson;   // 菜单数据

    QScopedPointer<VNVoiceBlock> voiceBlock { nullptr };   // 待另存的语音数据

    // 音频服务dbus接口 (org.deepin.dde.Audio)
    QDBusInterface *m_appearanceDBusInterface = nullptr;

    QStringList m_fontList;   // 字体列表
    QString m_defaultFont;   // 默认字体
};

#endif   // WEBENGINEHANDLER_H
