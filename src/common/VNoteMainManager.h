// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VNOTEMAINMANAGER_H
#define VNOTEMAINMANAGER_H

#include "vnotedatamanager.h"

#include <QVariantMap>

class VNoteMainManager : public QObject
{
    Q_OBJECT
public:
    VNoteMainManager();

    void init();
    void initQMLRegister();

    Q_INVOKABLE void vNoteFloderChanged(const int &index);
    Q_INVOKABLE void vNoteChanged(const int &index);

signals:
    void finishedFolderLoad(const QList<QVariantMap> &foldersData);
    void updateNotes(const QList<QVariantMap> &notesData);

private slots:
    void onVNoteFoldersLoaded();

private:
    void initData();
    void initConnections();
    int loadNotepads();
    int loadNotes(VNoteFolder *folder);

private:
    QList<VNoteItem*> m_noteItems;
};

#endif // VNOTEMAINMANAGER_H