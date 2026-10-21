// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vnoteelidepushbutton.h"

#include <QPainter>
#include <QStyleOptionButton>
#include <QStylePainter>

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

    QFontMetrics fm = fontMetrics();
    int textWidth = opt.rect.width() - 2 * style()->pixelMetric(QStyle::PM_ButtonMargin, &opt, this);
    if (textWidth < 0)
        textWidth = 0;

    QString elidedText = fm.elidedText(opt.text, Qt::ElideRight, textWidth);
    opt.text = elidedText;

    painter.drawControl(QStyle::CE_PushButton, opt);
}
