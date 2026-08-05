// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UT_VOICETOTEXTTASKMANAGER_H
#define UT_VOICETOTEXTTASKMANAGER_H

#include "gtest/gtest.h"
#include <QTest>
#include <QObject>

class UT_VoiceToTextTaskManager : public QObject
    , public ::testing::Test
{
    Q_OBJECT
public:
    UT_VoiceToTextTaskManager();

protected:
    virtual void SetUp() override;
    virtual void TearDown() override;
};

#endif // UT_VOICETOTEXTTASKMANAGER_H
