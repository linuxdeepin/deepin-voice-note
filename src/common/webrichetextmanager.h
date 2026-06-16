// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef WEBRICHTEXTMANAGER_H
#define WEBRICHTEXTMANAGER_H

#include "vnoteitem.h"

#include <QObject>


class WebRichTextManager : public QObject
{
    Q_OBJECT
public:
    explicit WebRichTextManager(QObject *parent = nullptr);

    void initData(VNoteItem *data, const QString reg, bool focus = false);
    void initConnect();

    void clearJSContent();

    void initUpdateTimer();
    bool hasPendingTextChange() const;
    int pendingTextChangeNoteId() const;
    int currentNoteId() const;
    void requestUpdateNoteNow();

public slots:
    void onLoadFinsh();

    void onSetDataFinsh();

    void updateNote();

    void onUpdateNoteWithResult(VNoteItem *data, const QString &result);

    void insertVoiceItem(const QString &voicePath, qint64 voiceSize);

signals:
    void needUpdateNote(int noteId);
    void noteTextChanged();
    void updateSearch();
    void scrollChange(const bool &isTop);
    void finishedUpdateNote();

private:
    void setData(VNoteItem *data, const QString reg);

private:
    VNoteItem *m_noteData {nullptr};
    QTimer *m_updateTimer {nullptr};

    bool m_textChange {false};
    int m_textChangeNoteId {-1};
    bool m_updateInProgress {false};
    int m_updateRequestNoteId {-1};
    quint64 m_textChangeSerial {0};
    quint64 m_updateRequestSerial {0};
    QPoint m_mouseClickPos {-1, -1}; //鼠标点击位置
    bool m_setFocus {false}; //是否设置焦点
    //右键菜单
    bool m_loadFinshSign = false; //后台与web通信连通标志 true: 连通， false: 未联通
};

#endif // WEBRICHTEXTMANAGER_H
