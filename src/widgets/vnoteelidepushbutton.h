// Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VNOTEELIDEPUSHBUTTON_H
#define VNOTEELIDEPUSHBUTTON_H

#include <DPushButton>

DWIDGET_USE_NAMESPACE

class VNoteElidePushButton : public DPushButton
{
    Q_OBJECT
public:
    explicit VNoteElidePushButton(QWidget *parent = nullptr);
    explicit VNoteElidePushButton(const QString &text, QWidget *parent = nullptr);
    ~VNoteElidePushButton() override {}

protected:
    void paintEvent(QPaintEvent *event) override;
};

#endif // VNOTEELIDEPUSHBUTTON_H
