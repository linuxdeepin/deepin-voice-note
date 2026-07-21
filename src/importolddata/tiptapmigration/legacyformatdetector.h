// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LEGACYFORMATDETECTOR_H
#define LEGACYFORMATDETECTOR_H

#include <QString>

enum class LegacyFormat {
    TiptapEnvelope,
    ProseMirrorDoc,
    LegacyHtmlCode,
    LegacyNoteDatas,
    PlainText,
    Invalid
};

class LegacyFormatDetector
{
public:
    static LegacyFormat detect(const QString &payload);
    static QString formatName(LegacyFormat format);

private:
    static bool looksLikeJson(const QString &payload);
    static bool looksLikeHtml(const QString &payload);
};

#endif // LEGACYFORMATDETECTOR_H
