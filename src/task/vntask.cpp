// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vntask.h"

/**
 * @brief VNTask::VNTask
 * @param parent
 */
VNTask::VNTask(QObject *parent)
    : QObject(parent)
    , QRunnable()
{
}
