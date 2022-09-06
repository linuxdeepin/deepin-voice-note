// Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UT_VNWAVEFORM_H
#define UT_VNWAVEFORM_H

#include "gtest/gtest.h"
#include <QTest>
#include <QObject>

class UT_VNWaveform : public QObject
    , public ::testing::Test
{
    Q_OBJECT
public:
    UT_VNWaveform();
};

#endif // UT_VNWAVEFORM_H
