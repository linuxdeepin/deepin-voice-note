// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vnoteelidepushbutton.h"

#include <QFontMetrics>
#include <QStylePainter>
#include <QStyleOptionButton>

VNoteElidePushButton::VNoteElidePushButton(QWidget *parent)
    : DPushButton(parent)
{
}

VNoteElidePushButton::VNoteElidePushButton(const QString &text, QWidget *parent)
    : DPushButton(text, parent)
{
}

void VNoteElidePushButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QStylePainter painter(this);
    QStyleOptionButton opt;
    initStyleOption(&opt);

    QString fullText = text();
    QFontMetrics fm = fontMetrics();
    int availableWidth = rect().width() - 2 * style()->pixelMetric(QStyle::PM_ButtonMargin, &opt, this);
    QString elidedText = fm.elidedText(fullText, Qt::ElideRight, availableWidth);

    opt.text = elidedText;
    painter.drawControl(QStyle::CE_PushButton, opt);
}
