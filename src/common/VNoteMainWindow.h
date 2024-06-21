// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VNOTEMAINWINDOW_H
#define VNOTEMAINWINDOW_H

#include "vnotedatamanager.h"

class VNoteMainWindow : public QObject
{
    Q_OBJECT
public:
    VNoteMainWindow();

signals:
    void finishedFolderLoad(QStringList folders);

private slots:
    void onVNoteFoldersLoaded();

private:
    void initData();
    void initConnections();
    int loadNotepads();
};

#endif // VNOTEMAINWINDOW_H