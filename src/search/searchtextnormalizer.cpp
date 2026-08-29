// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "searchtextnormalizer.h"

QString SearchTextNormalizer::normalize(const QString &text)
{
    QString result = text.normalized(QString::NormalizationForm_KC).toCaseFolded();

    bool previousWasSpace = false;
    QString compacted;
    compacted.reserve(result.size());
    for (const QChar ch : result) {
        if (ch.isSpace() || ch.category() == QChar::Other_Control) {
            if (!previousWasSpace && !compacted.isEmpty()) {
                compacted.append(QLatin1Char(' '));
            }
            previousWasSpace = true;
            continue;
        }
        compacted.append(ch);
        previousWasSpace = false;
    }

    if (compacted.endsWith(QLatin1Char(' '))) {
        compacted.chop(1);
    }
    return compacted;
}
