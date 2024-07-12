// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef WEBENGINEHANDLER_H
#define WEBENGINEHANDLER_H

#include <QObject>
#include <QRect>

class QDBusInterface;

class WebEngineHandler : public QObject
{
    Q_OBJECT
public:
    explicit WebEngineHandler(QObject *parent = nullptr);

public Q_SLOTS:
    void onThemeChanged();

private:
    void initFontsInformation();

private:
    QRect m_editToolbarRect;

    // 音频服务dbus接口 (org.deepin.dde.Audio)
    QDBusInterface *m_appearanceDBusInterface = nullptr;

    QStringList m_fontList; // 字体列表
    QString m_defaultFont;  // 默认字体
};

#endif   // WEBENGINEHANDLER_H
