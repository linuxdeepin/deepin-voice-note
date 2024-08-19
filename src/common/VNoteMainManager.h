// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VNOTEMAINMANAGER_H
#define VNOTEMAINMANAGER_H

#include "vnotedatamanager.h"
#include "vnoteforlder.h"
#include "vnoteitem.h"

#include <QVariantMap>

class WebRichTextManager;
class VNoteMainManager : public QObject
{
    Q_OBJECT
public:
    enum SaveAsType {
        Note = 0, //保存笔记
        Text, //txt类型
        Html, //html类型
        Voice, //语音类型
    };
    Q_ENUM(SaveAsType)

    static VNoteMainManager* instance();
    void initNote();
    void initQMLRegister();

    Q_INVOKABLE void vNoteFloderChanged(const int &index);
    Q_INVOKABLE void vNoteChanged(const int &index);
    Q_INVOKABLE void vNoteCreateFolder();
    Q_INVOKABLE void vNoteDeleteFolder(const int &index);
    Q_INVOKABLE void createNote();
    Q_INVOKABLE void deleteNote(const QList<int> &index);
    Q_INVOKABLE void moveNotes(const QVariantList &index, const int &folderIndex);
    Q_INVOKABLE void saveAs(const QVariantList &index, const QString &path, const SaveAsType type = Note);
    Q_INVOKABLE void updateTop(const int &index, const bool &top);
    Q_INVOKABLE bool getTop();
    Q_INVOKABLE void updateSort(const int &src, const int &dst);
    Q_INVOKABLE void renameFolder(const int &index, const QString &name);
    Q_INVOKABLE QList<QVariantMap> sortNoteList(const QList<QVariantMap> &dataList);
    Q_INVOKABLE void vNoteSearch(const QString &text);
    Q_INVOKABLE int loadSearchNotes(const QString &key);
    
    

signals:
    void finishedFolderLoad(const QList<QVariantMap> &foldersData);
    void updateNotes(const QList<QVariantMap> &notesData);
    void addNoteAtHead(const QVariantMap &noteData);
    void addFolderFinished(const QVariantMap &folderData);
    void noSearchResult();

private slots:
    void onVNoteFoldersLoaded();
    void onExportFinished(int err);

private:
    VNoteMainManager();
    void initData();
    void initConnections();
    int loadNotepads();
    int loadNotes(VNoteFolder *folder);

    VNoteFolder* getFloderByIndex(const int &index);
    VNoteItem* getNoteByIndex(const int &index);

private:
    int m_currentFolderIndex {-1};
    int m_currentHasTop {0};
    QList<VNoteItem*> m_noteItems;
    QStringList m_folderSort;
    WebRichTextManager *m_richTextManager {nullptr};
    QString m_searchText;
};

#endif // VNOTEMAINMANAGER_H
