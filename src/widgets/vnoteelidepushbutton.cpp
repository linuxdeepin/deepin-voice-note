// Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vnoteelidepushbutton.h"

#include <QFontMetrics>
#include <QStyle>
#include <QStyleOptionButton>
#include <QStylePainter>

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
    QStyleOptionButton option;
    initStyleOption(&option);

    QRect contentRect = style()->subElementRect(QStyle::SE_PushButtonContents, &option, this);
    option.text = fontMetrics().elidedText(option.text, Qt::ElideRight, contentRect.width());

    painter.drawControl(QStyle::CE_PushButton, option);
}
