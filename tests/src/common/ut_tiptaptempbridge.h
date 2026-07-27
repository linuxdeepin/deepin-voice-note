// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UT_TIPTAPTEMPBRIDGE_H
#define UT_TIPTAPTEMPBRIDGE_H

#include "gtest/gtest.h"
#include <QTest>
#include <QObject>

// TTP-022: 临时 Tiptap 通道桥接单元测试
class UT_TiptapTempBridge : public QObject
    , public ::testing::Test
{
    Q_OBJECT
public:
    UT_TiptapTempBridge();
};

#endif // UT_TIPTAPTEMPBRIDGE_H
