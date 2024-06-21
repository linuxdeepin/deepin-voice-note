// Copyright (C) 2020 ~ 2020 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common/vnotedatamanager.h"
#include "common/VNoteMainManager.h"
#include "common/jscontent.h"

#include <QQmlApplicationEngine>
#include <QScopedPointer>
#include <QQmlContext>
#include <QStandardPaths>
#include <QIcon>
#include <QDBusInterface>
#include <QDBusReply>

#include <QGuiApplication>
#include <QCommandLineParser>

#include <QQmlApplicationEngine>
#include <QtWebEngineQuick/qtwebenginequickglobal.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

QObject *jsContent_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    return JsContent::instance();
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    VNoteMainManager mainManager;
    mainManager.initQMLRegister();
    QtWebEngineQuick::initialize();

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);   
    engine.load(url);
    mainManager.init();

    return app.exec();
}
