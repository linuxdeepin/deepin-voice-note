// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "uosaicapabilitymanager.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDebug>

namespace {
constexpr int kDbusTimeoutMs = 2000;
const QString kCopilotService = QStringLiteral("com.deepin.copilot");
const QString kCopilotPath = QStringLiteral("/com/deepin/copilot");
const QString kCopilotInterface = QStringLiteral("com.deepin.copilot");
const QString kAiMainPath = QStringLiteral("/aiassistant/deepinmain");
const QString kAiMainInterface = QStringLiteral("com.iflytek.aiassistant.mainWindow");
const QString kAiIatPath = QStringLiteral("/aiassistant/iat");
const QString kAiIatInterface = QStringLiteral("com.iflytek.aiassistant.iat");
const QString kAppStoreService = QStringLiteral("com.home.appstore.client");
const QString kAppStorePath = QStringLiteral("/com/home/appstore/client");
const QString kAppStoreInterface = QStringLiteral("com.home.appstore.client");
const QString kUosAiAppStoreUri = QStringLiteral("app_detail_info/uos-ai");
}

UosAiCapabilityManager *UosAiCapabilityManager::instance()
{
    static UosAiCapabilityManager manager;
    return &manager;
}

UosAiCapabilityManager::Status UosAiCapabilityManager::checkCapability(Capability capability) const
{
    if (!isUosAiInstalled()) {
        qWarning() << "UOS AI is not installed or cannot be reached";
        return NotInstalled;
    }

    switch (capability) {
    case SpeechToText:
        if (hasDbusMethod(kCopilotService, kAiMainPath, kAiMainInterface, QStringLiteral("SpeechToText"))
            && hasDbusMethod(kCopilotService, kAiIatPath, kAiIatInterface, QStringLiteral("getIatEnable"))) {
            return Available;
        }
        break;
    case AudioToText:
        if (hasDbusMethod(kCopilotService, kAiMainPath, kAiMainInterface, QStringLiteral("startAsr"))) {
            return Available;
        }
        break;
    }

    qWarning() << "UOS AI does not support requested capability:" << capability;
    return UnsupportedVersion;
}

bool UosAiCapabilityManager::isUpdateRequired(Status status) const
{
    return status == NotInstalled || status == UnsupportedVersion;
}

bool UosAiCapabilityManager::openUosAiInAppStore() const
{
    QDBusInterface appStore(kAppStoreService,
                            kAppStorePath,
                            kAppStoreInterface,
                            QDBusConnection::sessionBus());
    appStore.setTimeout(kDbusTimeoutMs);
    if (!appStore.isValid()) {
        qWarning() << "App Store D-Bus interface is invalid:" << appStore.lastError().message();
        return false;
    }

    QDBusMessage reply = appStore.call(QStringLiteral("openBusinessUri"), kUosAiAppStoreUri);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qWarning() << "Failed to open UOS AI detail in App Store:" << reply.errorMessage();
        return false;
    }

    qInfo() << "Opened UOS AI detail in App Store with uri:" << kUosAiAppStoreUri;
    return true;
}

bool UosAiCapabilityManager::isUosAiInstalled() const
{
    QDBusInterface copilot(kCopilotService,
                           kCopilotPath,
                           kCopilotInterface,
                           QDBusConnection::sessionBus());
    copilot.setTimeout(kDbusTimeoutMs);

    QDBusReply<QString> version = copilot.call(QStringLiteral("version"));
    if (!version.isValid()) {
        qWarning() << "Failed to query UOS AI version:" << version.error().message();
        return false;
    }

    qDebug() << "Detected UOS AI version:" << version.value();
    return true;
}

bool UosAiCapabilityManager::hasDbusMethod(const QString &service,
                                           const QString &path,
                                           const QString &interface,
                                           const QString &method) const
{
    QDBusMessage message = QDBusMessage::createMethodCall(service,
                                                          path,
                                                          QStringLiteral("org.freedesktop.DBus.Introspectable"),
                                                          QStringLiteral("Introspect"));
    QDBusMessage reply = QDBusConnection::sessionBus().call(message, QDBus::Block, kDbusTimeoutMs);
    if (reply.type() == QDBusMessage::ErrorMessage || reply.arguments().isEmpty()) {
        qWarning() << "Failed to introspect UOS AI D-Bus object:" << service << path << reply.errorMessage();
        return false;
    }

    const QString xml = reply.arguments().constFirst().toString();
    const bool interfaceFound = xml.contains(QStringLiteral("interface name=\"") + interface + QLatin1Char('"'));
    const bool methodFound = xml.contains(QStringLiteral("method name=\"") + method + QLatin1Char('"'));
    if (!interfaceFound || !methodFound) {
        qWarning() << "UOS AI D-Bus method not found:" << interface << method << "at" << path;
        return false;
    }

    return true;
}
