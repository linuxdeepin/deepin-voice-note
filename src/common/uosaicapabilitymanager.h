// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UOSAICAPABILITYMANAGER_H
#define UOSAICAPABILITYMANAGER_H

#include <QString>

class UosAiCapabilityManager
{
public:
    enum Capability {
        SpeechToText,
        AudioToText,
    };

    enum Status {
        Available,
        NotInstalled,
        UnsupportedVersion,
    };

    static UosAiCapabilityManager *instance();

    Status checkCapability(Capability capability) const;
    bool isUpdateRequired(Status status) const;
    bool openUosAiInAppStore() const;

private:
    UosAiCapabilityManager() = default;

    bool isUosAiInstalled() const;
    bool hasDbusMethod(const QString &service,
                       const QString &path,
                       const QString &interface,
                       const QString &method) const;
};

#endif // UOSAICAPABILITYMANAGER_H
