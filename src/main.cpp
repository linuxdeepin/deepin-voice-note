// Copyright (C) 2020 ~ 2020 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common/vnotedatamanager.h"
#include "common/VNoteMainManager.h"
#include "common/jscontent.h"
#include "common/imageprovider.h"
#include "common/vtextspeechandtrmanager.h"

#include "config.h"

#include <QQmlApplicationEngine>
#include <QScopedPointer>
#include <QQmlContext>
#include <QStandardPaths>
#include <QIcon>
#include <QDBusInterface>
#include <QDBusReply>

#include <QGuiApplication>
#include <QApplication>
#include <QCommandLineParser>
#include <QTimer>

// 条件编译：根据 Qt 版本包含不同的 WebEngine 头文件
#ifdef USE_QT5
#include <QtWebEngine/QtWebEngine>
#else
#include <QtWebEngineQuick/qtwebenginequickglobal.h>
#endif

#include <DApplication>
#include <DGuiApplicationHelper>
#include <DLog>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE

int main(int argc, char *argv[])
{
#ifdef __mips64
    // MIPS64 特殊处理：设置环境变量以避免 DTK 配置系统的线程问题
    qputenv("DTK_DISABLE_DCONFIG_CACHE", "1");
    qputenv("DTK_FORCE_SYNC_DCONFIG", "1");
    qDebug() << "MIPS64: Applied DTK configuration workarounds";
#endif

    DApplication *app = new DApplication(argc, argv);

    qInfo() << "Starting deepin-voice-note application...";
    
#ifdef __mips64
    // MIPS64: 延迟单实例检查，避免早期的线程问题
    QTimer::singleShot(100, [app]() {
        DGuiApplicationHelper::instance()->setSingleInstanceInterval(-1);
        if (!DGuiApplicationHelper::instance()->setSingleInstance(app->applicationName(), DGuiApplicationHelper::UserScope)) {
            qWarning() << "Another instance of deepin-voice-note is already running";
            QCoreApplication::exit(0);
        }
    });
#else
    DGuiApplicationHelper::instance()->setSingleInstanceInterval(-1);
    if (!DGuiApplicationHelper::instance()->setSingleInstance(app->applicationName(), DGuiApplicationHelper::UserScope)) {
        qWarning() << "Another instance of deepin-voice-note is already running";
        return 0;
    }
#endif

    // TODO: The DTK theme color is initialized abnormally, and the application is temporarily circumvented
    DGuiApplicationHelper::instance()->paletteType();

    app->setOrganizationName("deepin");
    app->setApplicationName("deepin-voice-note");
    app->setApplicationVersion(VERSION);
    qInfo() << "Application initialized with version:" << VERSION;

    qputenv("QTWEBENGINE_REMOTE_DEBUGGING", "7777");
    VNoteMainManager::instance()->initQMLRegister();
    
    // 条件编译：根据 Qt 版本使用不同的 WebEngine 初始化方法
#ifdef USE_QT5
    QtWebEngine::initialize();
#else
    QtWebEngineQuick::initialize();
#endif
    
    ImageProvider *imageProvider = ImageProvider::instance();
    qInfo() << "QML and WebEngine initialized successfully";

    app->loadTranslator();
    app->setApplicationDisplayName(QObject::tr("deepin-voice-note"));
    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    engine.addImageProvider("Provider", imageProvider);
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) {
                qCritical() << "Failed to create QML object from URL:" << url.toString();
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection);
    engine.load(url);
    qInfo() << "QML engine loaded successfully";

    VNoteMainManager::instance()->initNote();
    qInfo() << "Note manager initialized";

    DLogManager::registerConsoleAppender();
    DLogManager::registerFileAppender();
    qInfo() << "Log system initialized";

    VTextSpeechAndTrManager::instance()->checkUosAiExists();
    qInfo() << "UOS AI check completed";
    
    return app->exec();
}
