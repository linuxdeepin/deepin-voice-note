// Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UT_LEFTVIEWDELEGATE_H
#define UT_LEFTVIEWDELEGATE_H

#include "gtest/gtest.h"
#include <QTest>
#include <QObject>

class UT_LeftViewDelegate : public QObject
    , public ::testing::Test
{
    Q_OBJECT
public:
    UT_LeftViewDelegate();
};

#endif // UT_LEFTVIEWDELEGATE_H
