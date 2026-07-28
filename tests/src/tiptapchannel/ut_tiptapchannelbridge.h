// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UT_TIPTAPCHANNELBRIDGE_H
#define UT_TIPTAPCHANNELBRIDGE_H

#include "gtest/gtest.h"
#include <QTest>
#include <QObject>

// 正式 TiptapChannelBridge 单元测试
class UT_TiptapChannelBridge : public QObject
    , public ::testing::Test
{
    Q_OBJECT
public:
    UT_TiptapChannelBridge();
};

#endif // UT_TIPTAPCHANNELBRIDGE_H
