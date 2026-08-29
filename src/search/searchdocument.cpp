// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "searchdocument.h"

QString searchFieldName(SearchField field)
{
    switch (field) {
    case SearchField::Title:
        return QStringLiteral("title");
    case SearchField::Body:
        return QStringLiteral("body");
    case SearchField::VoiceTitle:
        return QStringLiteral("voiceTitle");
    case SearchField::VoiceTranscript:
        return QStringLiteral("voiceTranscript");
    case SearchField::ImageAlt:
        return QStringLiteral("imageAlt");
    }
    return QStringLiteral("body");
}
