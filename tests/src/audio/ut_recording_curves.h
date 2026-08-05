// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UT_RECORDING_CURVES_H
#define UT_RECORDING_CURVES_H

#include "gtest/gtest.h"
#include <QTest>
#include <QObject>

class UT_RecordingCurves : public QObject
    , public ::testing::Test
{
    Q_OBJECT
public:
    UT_RecordingCurves();
};

#endif // UT_RECORDING_CURVES_H
