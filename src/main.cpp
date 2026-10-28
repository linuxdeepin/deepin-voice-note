// Copyright (C) 2020 - 2026 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common/vnotedatamanager.h"
#include "common/logger.h"
#include "ddlog.h"
#include "common/VNoteMainManager.h"
#include "common/jscontent.h"
#include "common/imageprovider.h"
#include "common/vtextspeechandtrmanager.h"
#include "common/tiptapchannelbridge.h"

#include "importolddata/dbmigration/migrationorchestrator.h"
#include "common/migrationviewcontroller.h"
#include "config.h"

#include <QQmlApplicationEngine>
#include <QWindow>
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

#include <DApplication>
#include <DGuiApplicationHelper>
#include <DPlatformHandle>
#include <DLog>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE
DGUI_USE_NAMESPACE

static void activateExistingInstanceViaDBus()
{
    QDBusInterface iface("org.deepin.voicenote",
                         "/org/deepin/voicenote",
                         "org.deepin.voicenote",
                         QDBusConnection::sessionBus());
    if (iface.isValid()) {
        qCInfo(voiceNoteLog) << "Calling ActivateWindow on existing instance via D-Bus";
        iface.asyncCall("ActivateWindow");
    } else {
        qCWarning(voiceNoteLog) << "D-Bus interface not available:" << QDBusConnection::sessionBus().lastError().message();
    }
}

int main(int argc, char *argv[])
{
    // set log format and register console and journal appenders
    // （参考 deepin-reader：日志格式中包含分类，便于按分类观察日志级别切换）
    const QString logFormat = "%{time}{yy-MM-ddTHH:mm:ss.zzz} [%{type:-7}] [%{category}] <%{function}:%{line}> %{message}";
    DLogManager::setLogFormat(logFormat);
#if (DTK_VERSION >= DTK_VERSION_CHECK(5, 6, 8, 0))
    DLogManager::registerJournalAppender();
#endif
    DLogManager::registerConsoleAppender();

    // 动态日志规则监听必须放在最前面（任何日志打印、DApplication 创建之前）：
    // 先读取并清除 QT_LOGGING_RULES 环境变量，再应用 DConfig log_rules，
    // 并监听 DConfig 变更实现运行中动态调整日志级别。
    MLogger logger;

#ifdef __mips64
    // MIPS64 特殊处理：设置环境变量以避免 DTK 配置系统的线程问题
    qputenv("DTK_DISABLE_DCONFIG_CACHE", "1");
    qputenv("DTK_FORCE_SYNC_DCONFIG", "1");
    qCDebug(voiceNoteLog) << "MIPS64: Applied DTK configuration workarounds";
#endif

#ifdef __sw_64__
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--js-flags=--jitless");
#endif

    DApplication *app = new DApplication(argc, argv);

    // register file appender must after Application instance created
    DLogManager::registerFileAppender();

    qCInfo(voiceNoteLog) << "Starting deepin-voice-note application...";
    
#ifdef __mips64
    // MIPS64: 延迟单实例检查，避免早期的线程问题
    QTimer::singleShot(100, [app]() {
        DGuiApplicationHelper::instance()->setSingleInstanceInterval(-1);
        if (!DGuiApplicationHelper::instance()->setSingleInstance(app->applicationName(), DGuiApplicationHelper::UserScope)) {
            qCWarning(voiceNoteLog) << "Another instance of deepin-voice-note is already running";
            activateExistingInstanceViaDBus();
            QCoreApplication::exit(0);
        }
    });
#else
    DGuiApplicationHelper::instance()->setSingleInstanceInterval(-1);
    if (!DGuiApplicationHelper::instance()->setSingleInstance(app->applicationName(), DGuiApplicationHelper::UserScope)) {
        qCWarning(voiceNoteLog) << "Another instance of deepin-voice-note is already running";
        activateExistingInstanceViaDBus();
        return 0;
    }
#endif

    // TODO: The DTK theme color is initialized abnormally, and the application is temporarily circumvented
    DGuiApplicationHelper::instance()->paletteType();

    app->setOrganizationName("deepin");
    app->setApplicationName("deepin-voice-note");
    app->setApplicationVersion(VERSION);
    qCInfo(voiceNoteLog) << "Application initialized with version:" << VERSION;

#ifndef NDEBUG
    qputenv("QTWEBENGINE_REMOTE_DEBUGGING", "7777");
#endif
    VNoteMainManager::instance()->initQMLRegister();
    
    ImageProvider *imageProvider = ImageProvider::instance();
    qCInfo(voiceNoteLog) << "QML types initialized successfully";

    app->loadTranslator();
    app->setApplicationDisplayName(QObject::tr("deepin-voice-note"));
    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    engine.addImageProvider("Provider", imageProvider);
    engine.rootContext()->setContextProperty(
        "hasExistingFolders", VNoteMainManager::hasExistingFolders());
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) {
                qCCritical(voiceNoteLog) << "Failed to create QML object from URL:" << url.toString();
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection);
    engine.load(url);

    if (!engine.rootObjects().isEmpty()) {
        if (auto *rootWindow = qobject_cast<QWindow *>(engine.rootObjects().constFirst())) {
            // KWin skips its opacity startup animation when startEffectType is 0.
            DPlatformHandle platformHandle(rootWindow);
            platformHandle.setWindowStartUpEffect(DPlatformHandle::EffectTypes());
            rootWindow->show();
            rootWindow->requestActivate();
        }
    }
    qCInfo(voiceNoteLog) << "QML engine loaded successfully";

    // Tiptap 已作为默认富文本编辑器，默认执行存量 Summernote 数据迁移。
    // 如需临时回退旧编辑器，可显式设置 DVN_TIPTAP_DISABLE=1。
    if (TiptapChannelBridge::instance()->tiptapEnabled()) {
        MigrationViewController::instance()->start();
    } else {
        qCInfo(voiceNoteLog) << "Tiptap migration skipped because Tiptap is disabled by environment";
    }

    qCInfo(voiceNoteLog) << "Log system initialized";

    return app->exec();
}
