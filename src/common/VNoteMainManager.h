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
#include <QEventLoop>

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
    Q_INVOKABLE void updateTop(const int &id, const bool &top);
    Q_INVOKABLE bool getTop();
    Q_INVOKABLE void updateSort(const int &src, const int &dst);
    Q_INVOKABLE void renameFolder(const int &index, const QString &name);
    Q_INVOKABLE void renameNote(const int &index, const QString &newName);
    Q_INVOKABLE void vNoteSearch(const QString &text);
    Q_INVOKABLE void updateNoteWithResult(const QString &result);
    Q_INVOKABLE int loadSearchNotes(const QString &key);
    Q_INVOKABLE int loadAudioSource();
    Q_INVOKABLE void changeAudioSource(const int &source);
    Q_INVOKABLE void insertImages(const QStringList &filePaths);
    Q_INVOKABLE void checkNoteVoice(const QVariantList &index);
    Q_INVOKABLE void clearSearch();
    Q_INVOKABLE bool isInSearchMode() const;
    Q_INVOKABLE void preViewShortcut(const QPointF &point);
    Q_INVOKABLE void showPrivacy();
    Q_INVOKABLE void resumeVoicePlayer();
    Q_INVOKABLE void forceExit(bool needWait = false);
    Q_INVOKABLE bool isVoiceToText();
    Q_INVOKABLE QString getSavedTextPath();
    Q_INVOKABLE QString getSavedVoicePath();
    Q_INVOKABLE void saveUserSelectedPath(const QString &path, const SaveAsType type);

signals:
    void finishedFolderLoad(const QList<QVariantMap> &foldersData);
    void updateNotes(const QList<QVariantMap> &notesData, const int &selectIndex);
    void addNoteAtHead(const QVariantMap &noteData);
    void addFolderFinished(const QVariantMap &folderData);
    void noSearchResult();
    void searchFinished(const QList<QVariantMap> &notesData, const QString &key);
    void moveFinished(const QVariantList &index, const int &srcFolderIndex, const int &dstFolderIndex);
    void needUpdateNote();
    void updateRichTextSearch(const QString &key);
    void scrollChange(const bool &isTop);
    void updateEditNote(const int &noteId, const QString &time);

private slots:
    void onVNoteFoldersLoaded();
    void onExportFinished(int err);
    void onNoteChanged();
    void updateSearch();
    void exitWithSave();

private:
    VNoteMainManager();
    void initData();
    void initConnections();
    int loadNotepads();
    int loadNotes(VNoteFolder *folder);
    void insertVoice(const QString &path, qint64 size);
    void loadSettings();

    VNoteFolder* getFloderByIndex(const int &index);
    VNoteFolder* getFloderById(const int &id);
    int getFloderIndexById(const int &id);
    VNoteItem* getNoteById(const int &id);
    VNoteItem* deleteNoteById(const int &id);

private:
    int m_currentFolderIndex {-1};
    int m_currentNoteId {-1};
    int m_currentHasTop {0};
    QList<VNoteItem*> m_noteItems;
    QStringList m_folderSort;
    WebRichTextManager *m_richTextManager {nullptr};
    QString m_searchText;
    QEventLoop m_eventloop;
};

#endif // VNOTEMAINMANAGER_H
