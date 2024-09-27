// Copyright (C) 2020 ~ 2020 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common/vnotedatamanager.h"
#include "common/VNoteMainManager.h"
#include "common/jscontent.h"
#include "common/imageprovider.h"

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

#include <QQmlApplicationEngine>
#include <QtWebEngineQuick/qtwebenginequickglobal.h>

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
    DApplication *app = new DApplication(argc, argv);

    DGuiApplicationHelper::instance()->setSingleInstanceInterval(-1);
    if (!DGuiApplicationHelper::instance()->setSingleInstance(
            app->applicationName(),
            DGuiApplicationHelper::UserScope)) {
        return 0;
    }

    app->setOrganizationName("deepin");
    app->setApplicationName("deepin-voice-note");
    app->setApplicationVersion(VERSION);

    qputenv("QTWEBENGINE_REMOTE_DEBUGGING", "7777");
    VNoteMainManager::instance()->initQMLRegister();
    QtWebEngineQuick::initialize();
    ImageProvider* imageProvider = ImageProvider::instance();

    app->loadTranslator();

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    engine.addImageProvider("Provider", imageProvider);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);   
    engine.load(url);
    VNoteMainManager::instance()->initNote();

    DLogManager::registerConsoleAppender();
    DLogManager::registerFileAppender();

    return app->exec();
}
