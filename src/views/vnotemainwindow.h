/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     liuyanga <liuyanga@uniontech.com>
*
* Maintainer: liuyanga <liuyanga@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef VNOTEMAINWINDOW_H
#define VNOTEMAINWINDOW_H

#include "common/datatypedef.h"
#include "globaldef.h"

#include <QShortcut>
#include <QStandardItem>
#include <QList>
#include <QDBusPendingReply>

#include <DMainWindow>
#include <DSearchEdit>
#include <DSplitter>
#include <DTextEdit>
#include <DStackedWidget>
#include <DLabel>
#include <DFloatingButton>
#include <DAnchors>
#include <DFloatingMessage>
#include <DScrollArea>

DWIDGET_USE_NAMESPACE

struct VNVoiceBlock;
class VNoteRecordBar;
class VNoteIconButton;
class VNoteAudioDeviceWatcher;
class VNoteA2TManager;
class LeftView;
class MiddleView;
class RightView;
class HomePage;
class SplashView;
class VoiceNoteItem;
class DBusLogin1Manager;
class VNMainWndDelayInitTask;
class UpgradeView;

class VNoteMainWindow : public DMainWindow, public OpsStateInterface
{
    Q_OBJECT
public:
    VNoteMainWindow(QWidget *parent = nullptr);
    virtual ~VNoteMainWindow() override;
    bool checkIfNeedExit();

    enum WindowType {
        WndSplashAnim,
        WndHomePage,
        WndNoteShow,
#ifdef IMPORT_OLD_VERSION_DATA
        //*******Upgrade old Db code here only********
        WndUpgrade,
#endif
        //Add other type at here.
    };
    enum SpecialStatus {
        InvalidStatus = 0,
        SearchStart,
        SearchEnd,
        PlayVoiceStart,
        PlayVoiceEnd,
        RecordStart,
        RecordEnd,
        VoiceToTextStart,
        VoiceToTextEnd
    };

protected:
    void initUI();
    void initData();
    void initAppSetting();
    void initConnections();
    void initShortcuts();
    void initTitleBar();
    void initMainView();
    void initLeftView();
    void initMiddleView();
    void initRightView();
    void initAudioWatcher();
    void initLogin1Manager();
    void holdHaltLock();
    void releaseHaltLock();
    void initDelayWork();
    void delayInitTasks();

#ifdef IMPORT_OLD_VERSION_DATA
    //*******Upgrade old Db code here only********
    void initUpgradeView();
#endif

    void initSpliterView(); //正常主窗口
    void initSplashView();  // Splash animation view
    void initEmptyFoldersView();
    void setSpecialStatus(SpecialStatus status);
    void switchWidget(WindowType type);

    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
signals:
    void sigDltSelectContant();

public slots:
    void onVNoteFoldersLoaded();
    void onVNoteFolderChange(const QModelIndex &current, const QModelIndex &previous);
    void onVNoteChange(const QModelIndex &previous);
    void onMenuAction(QAction *action);
    //Process all context menu states
    void onMenuAbout2Show();
    void onVNoteSearch();
    void onVNoteSearchTextChange(const QString &text);
    void onCursorChange(int height, bool mouseMove); //调整详情页滚动条
    void onStartRecord(const QString &path);//开始录音
    void onFinshRecord(const QString &voicePath, qint64 voiceSize); //结束录音
    void onRightViewVoicePlay(VNVoiceBlock *voiceData);
    void onRightViewVoicePause(VNVoiceBlock *voiceData);
    void onPlayPlugVoicePlay(VNVoiceBlock *voiceData);
    void onPlayPlugVoicePause(VNVoiceBlock *voiceData);
    void onPlayPlugVoiceStop(VNVoiceBlock *voiceData);
    void onNewNotebook();
    void onNewNote();

    void onChangeTheme();
    //Audio to text API
    void onA2TStart(bool first = true);
    void onA2TStartAgain();
    void onA2TError(int error);
    void onA2TSuccess(const QString &text);
    //Shotcuts slots
    void onPreviewShortcut();
    void initAsrErrMessage();
    void initDeviceExceptionErrMessage();
    void showAsrErrMessage(const QString &strMessage);
    void showDeviceExceptionErrMessage();
    void closeDeviceExceptionErrMessage();
    //System shutdon
    void onSystemDown(bool active);
private:
    //左侧列表视图操作相关
    void addNotepad();
    void editNotepad();
    void delNotepad();
    int loadNotepads();

    //中间列表视图操作
    void addNote();
    void editNote();
    void delNote();
    int loadNotes(VNoteFolder *folder);
    int loadSearchNotes(const QString &key);

    //Check if wen can do shortcuts
    bool canDoShortcutAction() const;
    void release();

private:
    DSearchEdit *m_noteSearchEdit {nullptr};

#ifdef TITLE_ACITON_PANEL
    //titlebar actions
    QWidget     *m_actionPanel {nullptr};
    DIconButton *m_addNewNoteBtn {nullptr};
#endif

#ifdef IMPORT_OLD_VERSION_DATA
    //*******Upgrade old Db code here only********
    bool        m_fNeedUpgradeOldDb = false;
#endif

    DScrollArea     *m_rightViewScrollArea {nullptr};
    QWidget         *m_rightViewHolder {nullptr};
    QWidget         *m_recordBarHolder {nullptr};
    QWidget         *m_leftViewHolder {nullptr};
    QWidget         *m_middleViewHolder {nullptr};
    QWidget         *m_centerWidget {nullptr};

    DSplitter   *m_mainWndSpliter {nullptr};
    LeftView    *m_leftView {nullptr};
    MiddleView  *m_middleView {nullptr};
    RightView   *m_rightView {nullptr};

    DPushButton *m_addNotepadBtn {nullptr};
    DFloatingButton *m_addNoteBtn {nullptr};

    VNoteRecordBar *m_recordBar {nullptr};
    //Audio device state watch thread
    VNoteAudioDeviceWatcher *m_audioDeviceWatcher {nullptr};
    VNoteA2TManager *m_a2tManager {nullptr};

    UpgradeView *m_upgradeView {nullptr};
    SplashView *m_splashView {nullptr};
    HomePage *m_wndHomePage {nullptr};
    DStackedWidget *m_stackedWidget {nullptr};
    bool            m_rightViewHasFouse {true};

    //Shortcuts key
    //*****************Shortcut key begin*********************
    QScopedPointer<QShortcut> m_stNewNotebook;   //Ctrl+N
    QScopedPointer<QShortcut> m_stRemNotebook;   //F2

    QScopedPointer<QShortcut> m_stNewNote;       //Ctrl+B
    QScopedPointer<QShortcut> m_stRemNote;       //F3

    QScopedPointer<QShortcut> m_stPlayorPause;   //Space
    QScopedPointer<QShortcut> m_stRecording;     //Ctrl+R
    QScopedPointer<QShortcut> m_stVoice2Text;    //Ctrl+W
    QScopedPointer<QShortcut> m_stSaveAsMp3;     //Ctrl+P
    QScopedPointer<QShortcut> m_stSaveAsText;    //Ctrl+S
    QScopedPointer<QShortcut> m_stSaveVoices;    //Ctrl+Y
    QScopedPointer<QShortcut> m_stDelete;    //Delete

    QScopedPointer<QShortcut> m_stPreviewShortcuts;
    //     Help                               //F1 DTK Implementaion
    //*****************Shortcut keys end**********************

    QString          m_searchKey;
    //VoiceNoteItem    *m_currentAsrVoice {nullptr};
    DFloatingMessage *m_asrErrMeassage {nullptr};
    DFloatingMessage *m_pDeviceExceptionMsg {nullptr};
    DPushButton      *m_asrAgainBtn {nullptr};
    //Login session manager
    DBusLogin1Manager *m_pLogin1Manager {nullptr};
    QDBusPendingReply<QDBusUnixFileDescriptor> m_lockFd;

    friend class VNMainWndDelayInitTask;
};


#endif // VNOTEMAINWINDOW_H
