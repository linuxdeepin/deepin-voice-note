// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VNOTEMAINMANAGER_H
#define VNOTEMAINMANAGER_H

#include "vnotedatamanager.h"

#include <QVariantMap>

class WebRichTextManager;
class VNoteMainManager : public QObject
{
    Q_OBJECT
public:
    static VNoteMainManager* instance();
    void initNote();
    void initQMLRegister();

    Q_INVOKABLE void vNoteFloderChanged(const int &index);
    Q_INVOKABLE void vNoteChanged(const int &index);
    Q_INVOKABLE void createNote();
    Q_INVOKABLE void deleteNote(const QList<int> &index);

signals:
    void finishedFolderLoad(const QList<QVariantMap> &foldersData);
    void updateNotes(const QList<QVariantMap> &notesData);
    void addNoteAtHead(const QVariantMap &noteData);

private slots:
    void onVNoteFoldersLoaded();

private:
    VNoteMainManager();
    void initData();
    void initConnections();
    int loadNotepads();
    int loadNotes(VNoteFolder *folder);

private:
    int m_currentFolderIndex {-1};
    QList<VNoteItem*> m_noteItems;
    WebRichTextManager *m_richTextManager {nullptr};
};

#endif // VNOTEMAINMANAGER_H