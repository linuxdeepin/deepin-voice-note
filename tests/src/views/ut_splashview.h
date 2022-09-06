// Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UT_SPLASHVIEW_H
#define UT_SPLASHVIEW_H

#include "gtest/gtest.h"
#include <QTest>
#include <QObject>

class UT_SplashView : public QObject
    , public ::testing::Test
{
    Q_OBJECT
public:
    UT_SplashView();
};

#endif // UT_SPLASHVIEW_H
