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

#include "views/vnotemainwindow.h"
#include "views/leftview.h"
#include "views/middleview.h"
#include "views/rightview.h"
#include "views/homepage.h"
#include "views/splashview.h"
#include "views/voicenoteitem.h"
#include "views/middleviewsortfilter.h"

#include "common/standarditemcommon.h"
#include "common/vnotedatamanager.h"
#include "common/vnotea2tmanager.h"
#include "common/vnoteitem.h"
#include "common/vnoteforlder.h"
#include "common/actionmanager.h"
#include "common/vnotedatasafefymanager.h"

#include "common/utils.h"
#include "common/actionmanager.h"
#include "common/setting.h"
#include "common/performancemonitor.h"
#include "common/jscontent.h"
#include "db/vnotefolderoper.h"
#include "db/vnoteitemoper.h"
#include "db/vnotedbmanager.h"

#include "dbus/dbuslogin1manager.h"

#include "dialog/vnotemessagedialog.h"
#include "views/vnoterecordbar.h"
#include "widgets/vnoteiconbutton.h"
#include "task/vnmainwnddelayinittask.h"
#include "views/webengineview.h"

#ifdef IMPORT_OLD_VERSION_DATA
#include "importolddata/upgradeview.h"
#include "importolddata/upgradedbutil.h"
#endif

#include "vnoteapplication.h"

#include <QScrollBar>
#include <QLocale>
#include <QDesktopServices>

#include <DApplication>
#include <DApplicationHelper>
#include <DFontSizeManager>
#include <DLog>
#include <DStatusBar>
#include <DTitlebar>
#include <DSettingsDialog>
#include <DSysInfo>

static OpsStateInterface *stateOperation = nullptr;

/**
 * @brief VNoteMainWindow::VNoteMainWindow
 * @param parent
 */
VNoteMainWindow::VNoteMainWindow(QWidget *parent)
    : DMainWindow(parent)
{
    //Init app data here
    initAppSetting();
    initUI();
    initConnections();
    // Request DataManager load  note folders
    initData();
    initShortcuts();
    initA2TManager();
    //Init the login manager
    initLogin1Manager();
    //Init delay task
    delayInitTasks();
}

/**
 * @brief VNoteMainWindow::~VNoteMainWindow
 */
VNoteMainWindow::~VNoteMainWindow()
{
    release();
}

/**
 * @brief VNoteMainWindow::initUI
 */
void VNoteMainWindow::initUI()
{
    initTitleBar();
    initMainView();
}

/**
 * @brief VNoteMainWindow::initData
 */
void VNoteMainWindow::initData()
{
    VNoteDataManager::instance()->reqNoteDefIcons();
    VNoteDataManager::instance()->reqNoteFolders();
    VNoteDataManager::instance()->reqNoteItems();
}

/**
 * @brief VNoteMainWindow::initAppSetting
 */
void VNoteMainWindow::initAppSetting()
{
    stateOperation = OpsStateInterface::instance();
}

/**
 * @brief VNoteMainWindow::initConnections
 */
void VNoteMainWindow::initConnections()
{
    connect(VNoteDataManager::instance(), &VNoteDataManager::onAllDatasReady,
            this, &VNoteMainWindow::onVNoteFoldersLoaded);

    connect(m_noteSearchEdit, &DSearchEdit::editingFinished,
            this, &VNoteMainWindow::onVNoteSearch);

    connect(m_noteSearchEdit, &DSearchEdit::textChanged,
            this, &VNoteMainWindow::onVNoteSearchTextChange);

    connect(m_leftView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &VNoteMainWindow::onVNoteFolderChange);

    connect(m_middleView, SIGNAL(currentChanged(const QModelIndex &)),
            this, SLOT(onVNoteChange(const QModelIndex &)));

//    connect(m_rightView, &RightView::contentChanged,
//            m_middleView, &MiddleView::onNoteChanged);

//    connect(m_rightView, &RightView::sigVoicePlay,
//            this, &VNoteMainWindow::onRightViewVoicePlay);
//    connect(m_rightView, &RightView::sigVoicePause,
//            this, &VNoteMainWindow::onRightViewVoicePause);
//    connect(m_rightView, &RightView::sigCursorChange,
//            this, &VNoteMainWindow::onCursorChange);

    connect(JsContent::instance(), &JsContent::voicePlay,
            this, &VNoteMainWindow::onRightViewVoicePlay);
    connect(JsContent::instance(), &JsContent::voicePause,
            this, &VNoteMainWindow::onRightViewVoicePause);

    connect(m_addNotepadBtn, &DPushButton::clicked,
            this, &VNoteMainWindow::onNewNotebook);

    connect(m_addNoteBtn, &VNoteIconButton::clicked,
            this, &VNoteMainWindow::onNewNote);

    connect(m_wndHomePage, &HomePage::sigAddFolderByInitPage,
            this, &VNoteMainWindow::addNotepad);

    connect(m_recordBar, &VNoteRecordBar::sigStartRecord,
            this, &VNoteMainWindow::onStartRecord);
    connect(m_recordBar, &VNoteRecordBar::sigFinshRecord,
            this, &VNoteMainWindow::onFinshRecord);
    connect(m_recordBar, &VNoteRecordBar::sigPlayVoice,
            this, &VNoteMainWindow::onPlayPlugVoicePlay);
    connect(m_recordBar, &VNoteRecordBar::sigPauseVoice,
            this, &VNoteMainWindow::onPlayPlugVoicePause);
    connect(m_recordBar, &VNoteRecordBar::sigWidgetClose,
            this, &VNoteMainWindow::onPlayPlugVoiceStop);
    connect(m_recordBar, &VNoteRecordBar::sigDeviceExceptionMsgShow,
            this, &VNoteMainWindow::showDeviceExceptionErrMessage);
    connect(m_recordBar, &VNoteRecordBar::sigDeviceExceptionMsgClose,
            this, &VNoteMainWindow::closeDeviceExceptionErrMessage);

    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this,
            &VNoteMainWindow::onChangeTheme);

    //Bind all context menu states handler
    connect(ActionManager::Instance()->notebookContextMenu(), &DMenu::aboutToShow,
            this, &VNoteMainWindow::onMenuAbout2Show);
    connect(ActionManager::Instance()->notebookContextMenu(), &DMenu::triggered,
            this, &VNoteMainWindow::onMenuAction);
    connect(ActionManager::Instance()->noteContextMenu(), &DMenu::aboutToShow,
            this, &VNoteMainWindow::onMenuAbout2Show);
    connect(ActionManager::Instance()->noteContextMenu(), &DMenu::triggered,
            this, &VNoteMainWindow::onMenuAction);
    connect(ActionManager::Instance()->detialContextMenu(), &DMenu::aboutToShow,
            this, &VNoteMainWindow::onMenuAbout2Show);
    connect(ActionManager::Instance()->detialContextMenu(), &DMenu::triggered,
            this, &VNoteMainWindow::onMenuAction);
}

/**
 * @brief VNoteMainWindow::initShortcuts
 */
void VNoteMainWindow::initShortcuts()
{
    //*******************LeftView Shortcuts****************************
    //Add notebook
    m_stNewNotebook.reset(new QShortcut(this));
    m_stNewNotebook->setKey(Qt::CTRL + Qt::Key_N);
    m_stNewNotebook->setContext(Qt::ApplicationShortcut);
    m_stNewNotebook->setAutoRepeat(false);

    connect(m_stNewNotebook.get(), &QShortcut::activated, this, [this] {
        if (!(stateOperation->isRecording()
                || stateOperation->isPlaying()
                || stateOperation->isVoice2Text()
                || stateOperation->isSearching()))
        {
            onNewNotebook();

            //If do shortcut in home page,need switch to note
            //page after add new notebook.
            if (!canDoShortcutAction()) {
                switchWidget(WndNoteShow);
            }
        }
    });

    //Rename notebook
    m_stRemNotebook.reset(new QShortcut(this));
    m_stRemNotebook->setKey(Qt::Key_F2);
    m_stRemNotebook->setContext(Qt::ApplicationShortcut);
    m_stRemNotebook->setAutoRepeat(false);

    connect(m_stRemNotebook.get(), &QShortcut::activated, this, [this] {
        if (canDoShortcutAction())
        {
            if (!stateOperation->isSearching()) {
                editNotepad();
            }
        }
    });

    //*******************MiddleView Shortcuts***************************
    //Add note
    m_stNewNote.reset(new QShortcut(this));
    m_stNewNote->setKey(Qt::CTRL + Qt::Key_B);
    m_stNewNote->setContext(Qt::ApplicationShortcut);
    m_stNewNote->setAutoRepeat(false);

    connect(m_stNewNote.get(), &QShortcut::activated, this, [this] {
        if (canDoShortcutAction()
                && !(stateOperation->isRecording()
                     || stateOperation->isPlaying()
                     || stateOperation->isVoice2Text()
                     || stateOperation->isSearching()))
        {
            onNewNote();
        }
    });

    //Rename note
    m_stRemNote.reset(new QShortcut(this));
    m_stRemNote->setKey(Qt::Key_F3);
    m_stRemNote->setContext(Qt::ApplicationShortcut);
    m_stRemNote->setAutoRepeat(false);

    connect(m_stRemNote.get(), &QShortcut::activated, this, [this] {
        if (canDoShortcutAction())
        {
            editNote();
        }
    });

    //*******************RightView Shortcuts*****************************
    //Play/Pause
    m_stPlayorPause.reset(new QShortcut(this));
    m_stPlayorPause->setKey(Qt::Key_Space);
    m_stPlayorPause->setContext(Qt::ApplicationShortcut);
    m_stPlayorPause->setAutoRepeat(false);

    connect(m_stPlayorPause.get(), &QShortcut::activated, this, [this] {
        if (canDoShortcutAction())
        {
            m_recordBar->playOrPauseVoice();
        }
    });

    //Add new recording
    m_stRecording.reset(new QShortcut(this));
    m_stRecording->setKey(Qt::CTRL + Qt::Key_R);
    m_stRecording->setContext(Qt::ApplicationShortcut);
    m_stRecording->setAutoRepeat(false);

    connect(m_stRecording.get(), &QShortcut::activated, this, [this] {
        if (canDoShortcutAction())
        {
            m_recordBar->onStartRecord();
        }
    });

    //Voice to Text
    m_stVoice2Text.reset(new QShortcut(this));
    m_stVoice2Text->setKey(Qt::CTRL + Qt::Key_W);
    m_stVoice2Text->setContext(Qt::ApplicationShortcut);
    m_stVoice2Text->setAutoRepeat(false);

    connect(m_stVoice2Text.get(), &QShortcut::activated, this, [this] {
        //Call method in rightview
        if (canDoShortcutAction())
        {
            if (!stateOperation->isVoice2Text() && stateOperation->isAiSrvExist()) {
                this->onA2TStart();
            }
        }
    });

    //Save as Mp3
    m_stSaveAsMp3.reset(new QShortcut(this));
    m_stSaveAsMp3->setKey(Qt::CTRL + Qt::Key_P);
    m_stSaveAsMp3->setContext(Qt::ApplicationShortcut);
    m_stSaveAsMp3->setAutoRepeat(false);

    connect(m_stSaveAsMp3.get(), &QShortcut::activated, this, [this] {
        //Call method in rightview
        Q_UNUSED(this);
        if (canDoShortcutAction())
        {
            m_rightView->saveMp3();
        }
    });

    //Save as Text
    m_stSaveAsText.reset(new QShortcut(this));
    m_stSaveAsText->setKey(Qt::CTRL + Qt::Key_S);
    m_stSaveAsText->setContext(Qt::ApplicationShortcut);
    m_stSaveAsText->setAutoRepeat(false);

    connect(m_stSaveAsText.get(), &QShortcut::activated, this, [this] {
        //Call method in rightview
        if (canDoShortcutAction())
        {
            VNoteItem *currNote = m_middleView->getCurrVNotedata();
            if (nullptr != currNote) {
                if (currNote->haveText()) {
                    m_middleView->saveAsText();
                }
            }
        }
    });

    //Save recordings
    m_stSaveVoices.reset(new QShortcut(this));
    m_stSaveVoices->setKey(Qt::CTRL + Qt::Key_Y);
    m_stSaveVoices->setContext(Qt::ApplicationShortcut);
    m_stSaveVoices->setAutoRepeat(false);

    connect(m_stSaveVoices.get(), &QShortcut::activated, this, [this] {
        //Call method in rightview
        if (canDoShortcutAction())
        {
            //Can't save recording when do recording.
            if (!stateOperation->isRecording()) {
                VNoteItem *currNote = m_middleView->getCurrVNotedata();
                if (nullptr != currNote) {
                    if (currNote->haveVoice()) {
                        m_middleView->saveRecords();
                    }
                }
            }
        }
    });

    //Notebook/Note/Detial delete key
    m_stDelete.reset(new QShortcut(this));
    m_stDelete->setKey(Qt::Key_Delete);
    m_stDelete->setContext(Qt::ApplicationShortcut);
    m_stDelete->setAutoRepeat(false);

    connect(m_stDelete.get(), &QShortcut::activated, this, [this] {
        if (canDoShortcutAction())
        {
            QAction *deleteAct = nullptr;

            /*
             * TODO:
             *     We check focus here to choice what action we will
             * take. Focus in leftview for delete notebook, in midlle
             * view for delete note, in Rightview for delete note content.
             * or do nothing.
             * */
            if (m_leftView->hasFocus()) {
                if (!stateOperation->isRecording()
                        && !stateOperation->isVoice2Text()
                        && !stateOperation->isPlaying()) {
                    deleteAct = ActionManager::Instance()->getActionById(
                        ActionManager::NotebookDelete);
                }
            } else if (m_middleView->hasFocus()) {
                if (!stateOperation->isRecording()
                        && !stateOperation->isVoice2Text()
                        && !stateOperation->isPlaying()
                        && m_middleView->count() > 0) {
                    deleteAct = ActionManager::Instance()->getActionById(
                                    ActionManager::NoteDelete);
                }
            } /*else if (m_rightView->hasFocus()) {
                deleteAct = ActionManager::Instance()->getActionById(
                    ActionManager::DetailDelete);
            }*/ else {
                QPoint pos = m_rightViewHolder->mapFromGlobal(QCursor::pos());
                if (m_rightViewHolder->rect().contains(pos)) {
                    deleteAct = ActionManager::Instance()->getActionById(
                                    ActionManager::DetailDelete);
                }
            }

            if (nullptr != deleteAct) {
                deleteAct->triggered();
            }
        }
    });

    m_stPreviewShortcuts.reset(new QShortcut(this));
    m_stPreviewShortcuts->setKey(QString("Ctrl+Shift+/"));
    m_stPreviewShortcuts->setContext(Qt::ApplicationShortcut);
    m_stPreviewShortcuts->setAutoRepeat(false);

    connect(m_stPreviewShortcuts.get(), &QShortcut::activated,
            this, &VNoteMainWindow::onPreviewShortcut);
}

/**
 * @brief VNoteMainWindow::initTitleBar
 */
void VNoteMainWindow::initTitleBar()
{
    titlebar()->setFixedHeight(VNOTE_TITLEBAR_HEIGHT);
    titlebar()->setTitle("");
    // Add logo
    titlebar()->setIcon(QIcon::fromTheme(DEEPIN_VOICE_NOTE));

#ifdef TITLE_ACITON_PANEL
    //Add action buttons
    m_actionPanel = new QWidget(this);

    QHBoxLayout *actionPanelLayout = new QHBoxLayout(m_actionPanel);
    actionPanelLayout->setSpacing(0);
    actionPanelLayout->setContentsMargins(0, 0, 0, 0);

    m_addNewNoteBtn = new DIconButton(m_actionPanel);
    m_addNewNoteBtn->setFixedSize(QSize(36, 36));

    actionPanelLayout->addSpacing(VNOTE_LEFTVIEW_W - 45);
    actionPanelLayout->addWidget(m_addNewNoteBtn, Qt::AlignLeft);

    titlebar()->addWidget(m_actionPanel, Qt::AlignLeft);
#endif
    initMenuExtension();
    titlebar()->setMenu(m_menuExtension);
    // Search note
    m_noteSearchEdit = new DSearchEdit(this);
    DFontSizeManager::instance()->bind(m_noteSearchEdit, DFontSizeManager::T6);
    m_noteSearchEdit->setFixedSize(QSize(VNOTE_SEARCHBAR_W, VNOTE_SEARCHBAR_H));
    m_noteSearchEdit->setPlaceHolder(DApplication::translate("TitleBar", "Search"));
    titlebar()->addWidget(m_noteSearchEdit);
}

/**
 * @brief VNoteMainWindow::initMainView
 */
void VNoteMainWindow::initMainView()
{
    initSpliterView();
    initSplashView();
    initEmptyFoldersView();
    m_centerWidget = new QWidget(this);

    m_stackedWidget = new DStackedWidget(m_centerWidget);
    m_stackedWidget->setContentsMargins(0, 0, 0, 0);
    m_stackedWidget->insertWidget(WndSplashAnim, m_splashView);
    m_stackedWidget->insertWidget(WndHomePage, m_wndHomePage);
    m_stackedWidget->insertWidget(WndNoteShow, m_mainWndSpliter);

    WindowType defaultWnd = WndSplashAnim;

#ifdef IMPORT_OLD_VERSION_DATA
    //*******Upgrade old Db code here only********
    initUpgradeView();
    m_stackedWidget->insertWidget(WndUpgrade, m_upgradeView);

    int upgradeState = UpgradeDbUtil::readUpgradeState();

    UpgradeDbUtil::checkUpdateState(upgradeState);

    m_fNeedUpgradeOldDb = UpgradeDbUtil::needUpdateOldDb(upgradeState);

    qInfo() << "Check if need upgrade old version:" << upgradeState
            << " isNeed:" << m_fNeedUpgradeOldDb;

    if (m_fNeedUpgradeOldDb) {
        defaultWnd = WndUpgrade;
    }
#endif
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_stackedWidget);
    m_centerWidget->setLayout(layout);

    switchWidget(defaultWnd);
    setCentralWidget(m_centerWidget);
    setTitlebarShadowEnabled(true);
}

/**
 * @brief VNoteMainWindow::initLeftView
 */
void VNoteMainWindow::initLeftView()
{
    m_leftViewHolder = new QWidget(m_mainWndSpliter);
    m_leftViewHolder->setObjectName("leftMainLayoutHolder");
    m_leftViewHolder->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_leftViewHolder->setFixedWidth(VNOTE_LEFTVIEW_W);
    m_leftViewHolder->setBackgroundRole(DPalette::Base);
    m_leftViewHolder->setAutoFillBackground(true);

    QVBoxLayout *leftHolderLayout = new QVBoxLayout();
    leftHolderLayout->setSpacing(0);
    leftHolderLayout->setContentsMargins(0, 5, 0, 0);
    m_leftView = new LeftView(m_leftViewHolder);

    //背景色透明，隐藏disable时的背景色
    DPalette pb = DApplicationHelper::instance()->palette(m_leftView);
    pb.setBrush(DPalette::Base, QColor(0, 0, 0, 0));
    m_leftView->setPalette(pb);

    m_leftView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_leftView->setContentsMargins(0, 0, 0, 0);
    m_leftView->setHeaderHidden(true);
    m_leftView->setFrameShape(QFrame::NoFrame);
    m_leftView->setItemsExpandable(false);
    m_leftView->setIndentation(0);
    QModelIndex notepadRootIndex = m_leftView->getNotepadRootIndex();
    m_leftView->expand(notepadRootIndex);
    leftHolderLayout->addWidget(m_leftView);

    m_addNotepadBtn = new DPushButton(DApplication::translate("VNoteMainWindow", "Create Notebook"),
                                      m_leftViewHolder);
    QVBoxLayout *btnLayout = new QVBoxLayout();
    btnLayout->addWidget(m_addNotepadBtn);
    btnLayout->setContentsMargins(10, 5, 10, 10);
    leftHolderLayout->addLayout(btnLayout, Qt::AlignHCenter);
    m_leftViewHolder->setLayout(leftHolderLayout);

#ifdef VNOTE_LAYOUT_DEBUG
    m_leftView->setStyleSheet("background: green");
#endif
}

/**
 * @brief VNoteMainWindow::initMiddleView
 */
void VNoteMainWindow::initMiddleView()
{
    m_middleViewHolder = new QWidget(m_mainWndSpliter);
    m_middleViewHolder->setObjectName("middleMainLayoutHolder");
    m_middleViewHolder->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_middleViewHolder->setFixedWidth(VNOTE_MIDDLEVIEW_W);
    m_middleViewHolder->setBackgroundRole(DPalette::Base);
    m_middleViewHolder->setAutoFillBackground(true);

    QVBoxLayout *middleHolderLayout = new QVBoxLayout();
    middleHolderLayout->setSpacing(0);
    middleHolderLayout->setContentsMargins(0, 5, 0, 5);

    m_middleView = new MiddleView(m_middleViewHolder);
    m_addNoteBtn = new DFloatingButton(DStyle::StandardPixmap::SP_IncreaseElement, m_middleViewHolder);
    m_addNoteBtn->setFixedSize(QSize(54, 54));
    m_addNoteBtn->raise();

    DAnchorsBase buttonAnchor(m_addNoteBtn);
    buttonAnchor.setAnchor(Qt::AnchorLeft, m_middleView, Qt::AnchorLeft);
    buttonAnchor.setAnchor(Qt::AnchorBottom, m_middleView, Qt::AnchorBottom);
    buttonAnchor.setBottomMargin(6);
    buttonAnchor.setLeftMargin(97);

    // ToDo:
    //    Add Left view widget here
    middleHolderLayout->addWidget(m_middleView);

    m_middleViewHolder->setLayout(middleHolderLayout);

#ifdef VNOTE_LAYOUT_DEBUG
    m_middleView->setStyleSheet("background: green");
#endif
}

/**
 * @brief VNoteMainWindow::initRightView
 */
void VNoteMainWindow::initRightView()
{
    m_rightViewHolder = new QWidget(m_mainWndSpliter);
    m_rightViewHolder->setObjectName("rightMainLayoutHolder");
    m_rightViewHolder->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_rightViewHolder->setBackgroundRole(DPalette::Base);
    m_rightViewHolder->setAutoFillBackground(true);

    QVBoxLayout *rightHolderLayout = new QVBoxLayout;
    rightHolderLayout->setSpacing(0);
    rightHolderLayout->setContentsMargins(0, 15, 10, 3);
    m_webView = new WebEngineView(m_rightViewHolder);
    rightHolderLayout->addWidget(m_webView);
#if 0
    m_rightViewScrollArea = new DScrollArea(m_rightViewHolder);
    m_rightView = new RightView(m_rightViewScrollArea);
    m_rightViewScrollArea->setLineWidth(0);
    m_rightViewScrollArea->setWidgetResizable(true);
    m_rightViewScrollArea->setWidget(m_rightView);
    rightHolderLayout->addWidget(m_rightViewScrollArea);
#endif
    //TODO:
    //    Add record area code here
    m_recordBarHolder = new QWidget(m_rightViewHolder);
    m_recordBarHolder->setFixedHeight(78);
    QVBoxLayout *recordBarHolderLayout = new QVBoxLayout(m_recordBarHolder);
    recordBarHolderLayout->setSpacing(0);
    recordBarHolderLayout->setContentsMargins(4, 1, 4, 0);

    m_recordBar = new VNoteRecordBar(m_recordBarHolder);
    m_recordBar->setBackgroundRole(DPalette::Base);
    m_recordBar->setAutoFillBackground(true);
    m_recordBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    recordBarHolderLayout->addWidget(m_recordBar);

    rightHolderLayout->addWidget(m_recordBarHolder);

    m_rightViewHolder->setLayout(rightHolderLayout);

    m_recordBar->setVisible(false);

#ifdef VNOTE_LAYOUT_DEBUG
    m_rightViewHolder->setStyleSheet("background: red");
    m_rightViewScrollArea->setStyleSheet("background: blue");
    m_recordBarHolder->setStyleSheet("background: yellow");
    m_recordBar->setStyleSheet("background: yellow");
#endif
}

/**
 * @brief VNoteMainWindow::initA2TManager
 */
void VNoteMainWindow::initA2TManager()
{
    //audio to text manager
    m_a2tManager = new VNoteA2TManager();

    //connect(m_rightView, &RightView::asrStart, this, &VNoteMainWindow::onA2TStart);
    connect(m_a2tManager, &VNoteA2TManager::asrError, this, &VNoteMainWindow::onA2TError);
    connect(m_a2tManager, &VNoteA2TManager::asrSuccess, this, &VNoteMainWindow::onA2TSuccess);
}

/**
 * @brief VNoteMainWindow::initLogin1Manager
 */
void VNoteMainWindow::initLogin1Manager()
{
    m_pLogin1Manager = new DBusLogin1Manager(
        "org.freedesktop.login1",
        "/org/freedesktop/login1",
        QDBusConnection::systemBus(), this);

    connect(m_pLogin1Manager, &DBusLogin1Manager::PrepareForSleep,
            this, &VNoteMainWindow::onSystemDown);
    connect(m_pLogin1Manager, &DBusLogin1Manager::PrepareForShutdown,
            this, &VNoteMainWindow::onSystemDown);
}

/**
 * @brief VNoteMainWindow::holdHaltLock
 */
void VNoteMainWindow::holdHaltLock()
{
    m_lockFd = m_pLogin1Manager->Inhibit(
                   "shutdown:sleep",
                   DApplication::translate("AppMain", "Voice Notes"),
                   DApplication::translate("AppMain", "Recordings not saved"),
                   "block");

    if (m_lockFd.isError()) {
        qCritical() << "Init login manager error:" << m_lockFd.error();
    }
}

/**
 * @brief VNoteMainWindow::releaseHaltLock
 */
void VNoteMainWindow::releaseHaltLock()
{
    QDBusPendingReply<QDBusUnixFileDescriptor> releaseLock = m_lockFd;
    m_lockFd = QDBusPendingReply<QDBusUnixFileDescriptor>();
}

/**
 * @brief VNoteMainWindow::initDelayWork
 */
void VNoteMainWindow::initDelayWork()
{
    ;
}

/**
 * @brief VNoteMainWindow::delayInitTasks
 */
void VNoteMainWindow::delayInitTasks()
{
    VNMainWndDelayInitTask *pMainWndDelayTask = new VNMainWndDelayInitTask(this);
    pMainWndDelayTask->setAutoDelete(true);
    pMainWndDelayTask->setObjectName("MainWindowDelayTask");

    QThreadPool::globalInstance()->start(pMainWndDelayTask);
}

/**
 * @brief VNoteMainWindow::onVNoteFoldersLoaded
 */
void VNoteMainWindow::onVNoteFoldersLoaded()
{
#ifdef IMPORT_OLD_VERSION_DATA
    if (m_fNeedUpgradeOldDb) {
        //Start upgrade if all components are ready.
        m_upgradeView->startUpgrade();
        return;
    }
#endif

    //If have folders show note view,else show
    //default home page
    if (loadNotepads() > 0) {
        m_leftView->setDefaultNotepadItem();
        switchWidget(WndNoteShow);
    } else {
        switchWidget(WndHomePage);
    }

    PerformanceMonitor::initializeAppFinish();
}

/**
 * @brief VNoteMainWindow::onVNoteSearch
 */
void VNoteMainWindow::onVNoteSearch()
{
    if (m_noteSearchEdit->lineEdit()->hasFocus()) {
        QString text = m_noteSearchEdit->text();
        if (!text.isEmpty()) {
            setSpecialStatus(SearchStart);
            m_searchKey = text;
            loadSearchNotes(m_searchKey);
        } else {
            setSpecialStatus(SearchEnd);
        }
    }
}

/**
 * @brief VNoteMainWindow::onVNoteSearchTextChange
 * @param text
 */
void VNoteMainWindow::onVNoteSearchTextChange(const QString &text)
{
    if (text.isEmpty()) {
        setSpecialStatus(SearchEnd);
    }
}

/**
 * @brief VNoteMainWindow::onVNoteFolderChange
 * @param current
 * @param previous
 */
void VNoteMainWindow::onVNoteFolderChange(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous);

    if (m_asrErrMeassage) {
        m_asrErrMeassage->setVisible(false);
    }

    VNoteFolder *data = static_cast<VNoteFolder *>(StandardItemCommon::getStandardItemData(current));
    if (!loadNotes(data)) {
        m_webView->initData(nullptr, m_searchKey, false);
        m_recordBar->setVisible(false);
    }
}

/**
 * @brief VNoteMainWindow::initSpliterView
 */
void VNoteMainWindow::initSpliterView()
{
    m_mainWndSpliter = new DSplitter(Qt::Horizontal, this);
    m_mainWndSpliter->setHandleWidth(2);

    initLeftView();
    initMiddleView();
    initRightView();

    // Disable spliter drag & resize
    QList<QWidget *> Children {m_middleViewHolder, m_rightViewHolder};

    for (auto it : Children) {
        QSplitterHandle *handle = m_mainWndSpliter->handle(m_mainWndSpliter->indexOf(it));
        if (handle) {
            handle->setFixedWidth(2);
            handle->setDisabled(true);

            //            DPalette pa = DApplicationHelper::instance()->palette(handle);
            //            QBrush splitBrush = pa.brush(DPalette::ItemBackground);
            //            pa.setBrush(DPalette::Background, splitBrush);
            //            handle->setPalette(pa);
            //            handle->setBackgroundRole(QPalette::Background);
            //            handle->setAutoFillBackground(true);
        }
    }
}

#ifdef IMPORT_OLD_VERSION_DATA
//*******Upgrade old Db code here only********

/**
 * @brief VNoteMainWindow::initUpgradeView
 */
void VNoteMainWindow::initUpgradeView()
{
    m_upgradeView = new UpgradeView(this);

    connect(m_upgradeView, &UpgradeView::upgradeDone,
    this, [this]() {
        //Clear the flag after upgrade & and call
        //the data loaded slot to refresh.
        m_fNeedUpgradeOldDb = false;

        onVNoteFoldersLoaded();

        qInfo() << "upgrade success.";
    },
    Qt::QueuedConnection);
}

#endif

/**
 * @brief VNoteMainWindow::initSplashView
 */
void VNoteMainWindow::initSplashView()
{
    m_splashView = new SplashView(this);
}

/**
 * @brief VNoteMainWindow::initEmptyFoldersView
 */
void VNoteMainWindow::initEmptyFoldersView()
{
    m_wndHomePage = new HomePage(this);
}

/**
 * @brief VNoteMainWindow::onStartRecord
 * @param path
 */
void VNoteMainWindow::onStartRecord(const QString &path)
{
    setSpecialStatus(RecordStart);

    //Add recording data safer.
    VNoteItem *currentNote = m_middleView->getCurrVNotedata();

    if (nullptr != currentNote && !path.isEmpty()) {
        VDataSafer safer;
        safer.setSaferType(VDataSafer::Safe);

        safer.folder_id = currentNote->folderId;
        safer.note_id = currentNote->noteId;
        safer.path = path;

        VNoteDataSafefyManager::instance()->doSafer(safer);
    } else {
        qCritical() << "UnSafe get currentNote data is null! Dangerous!!!" << path;
    }

    //Hold shutdown locker
    holdHaltLock();
}

/**
 * @brief VNoteMainWindow::onFinshRecord
 * @param voicePath
 * @param voiceSize
 */
void VNoteMainWindow::onFinshRecord(const QString &voicePath, qint64 voiceSize)
{
#if 1
    if (voiceSize >= 1000) {
        m_webView->insertVoiceItem(voicePath, voiceSize);

        //Recording normal,remove the data safer.
        VNoteItem *currentNote = m_middleView->getCurrVNotedata();

        if (nullptr != currentNote && !voicePath.isEmpty()) {
            VDataSafer safer;
            safer.setSaferType(VDataSafer::Unsafe);

            safer.folder_id = currentNote->folderId;
            safer.note_id = currentNote->noteId;
            safer.path = voicePath;

            VNoteDataSafefyManager::instance()->doSafer(safer);
        } else {
            qCritical() << "UnSafe get currentNote data is null! Dangerous!!!";
        }
    } else {
    }
    setSpecialStatus(RecordEnd);

    //Release shutdonw locker
    releaseHaltLock();

    if (stateOperation->isAppQuit()) {
        release();
        exit(0);
    }
#endif
}

/**
 * @brief VNoteMainWindow::onChangeTheme
 */
void VNoteMainWindow::onChangeTheme()
{
    ;
}

/**
 * @brief VNoteMainWindow::onA2TStartAgain
 */
void VNoteMainWindow ::onA2TStartAgain()
{
    onA2TStart(false);
}

/**
 * @brief VNoteMainWindow::onA2TStart
 * @param first true第一次转文字
 */
void VNoteMainWindow::onA2TStart(bool first)
{
    if (m_asrErrMeassage) {
        m_asrErrMeassage->setVisible(false);
    }
    VNoteBlock *data = JsContent::instance()->getCurrentBlock();
    if (data && data->getType() == VNoteBlock::Voice && data->blockText.isEmpty()) {
        if (nullptr != data) {
            //Check whether the audio lenght out of 20 minutes
            if (data->ptrVoice->voiceSize > MAX_A2T_AUDIO_LEN_MS) {
                VNoteMessageDialog audioOutLimit(
                    VNoteMessageDialog::AsrTimeLimit, this);

                audioOutLimit.exec();
            } else {
                setSpecialStatus(VoiceToTextStart);
                emit JsContent::instance()->setVoiceToText(data->blockid, DApplication::translate("VoiceNoteItem", "Converting voice to text"), 0);
                QTimer::singleShot(0, this, [this, data]() {
                    m_a2tManager->startAsr(data->ptrVoice->voicePath, data->ptrVoice->voiceSize);
                });
            }
        }
    }
}

/**
 * @brief VNoteMainWindow::onA2TError
 * @param error
 */
void VNoteMainWindow::onA2TError(int error)
{
    VNoteBlock *data = JsContent::instance()->getCurrentBlock();
    if (data && data->getType() == VNoteBlock::Voice) {
        emit JsContent::instance()->setVoiceToText(data->blockid, "", 0);
    }
    QString message = "";
    if (error == VNoteA2TManager::NetworkError) {
        message = DApplication::translate(
                      "VNoteErrorMessage",
                      "The voice conversion failed due to the poor network connection. "
                      "Do you want to try again?");
    } else {
        message = DApplication::translate(
                      "VNoteErrorMessage",
                      "The voice conversion failed. Do you want to try again?");
    }
    showAsrErrMessage(message);
    setSpecialStatus(VoiceToTextEnd);
}

/**
 * @brief VNoteMainWindow::onA2TSuccess
 * @param text
 */
void VNoteMainWindow::onA2TSuccess(const QString &text)
{
    VNoteBlock *data = JsContent::instance()->getCurrentBlock();
    if (data && data->getType() == VNoteBlock::Voice) {
        data->blockText = text;
        emit JsContent::instance()->setVoiceToText(data->blockid, text, 1);
    }
    setSpecialStatus(VoiceToTextEnd);
}

/**
 * @brief VNoteMainWindow::onPreviewShortcut
 */
void VNoteMainWindow::onPreviewShortcut()
{
    QRect rect = window()->geometry();
    QPoint pos(rect.x() + rect.width() / 2,
               rect.y() + rect.height() / 2);

    QJsonObject shortcutObj;
    QJsonArray jsonGroups;

    //******************************Notebooks**************************************************
    QMap<QString, QString> shortcutNotebookKeymap = {
        //Notebook
        {DApplication::translate("Shortcuts", "New notebook"), "Ctrl+N"},
        {DApplication::translate("Shortcuts", "Rename notebook"), "F2"},
        {DApplication::translate("Shortcuts", "Delete notebook"), "Delete"},
    };

    QJsonObject notebookJsonGroup;
    notebookJsonGroup.insert("groupName", DApplication::translate("ShortcutsGroups", "Notebooks"));
    QJsonArray notebookJsonItems;

    for (QMap<QString, QString>::iterator it = shortcutNotebookKeymap.begin();
            it != shortcutNotebookKeymap.end(); it++) {
        QJsonObject jsonItem;
        jsonItem.insert("name", it.key());
        jsonItem.insert("value", it.value().replace("Meta", "Super"));
        notebookJsonItems.append(jsonItem);
    }

    notebookJsonGroup.insert("groupItems", notebookJsonItems);
    jsonGroups.append(notebookJsonGroup);

    //******************************Notes**************************************************

    QMap<QString, QString> shortcutNoteKeymap = {
        //Note
        {DApplication::translate("Shortcuts", "New note"), "Ctrl+B"},
        {DApplication::translate("Shortcuts", "Rename note"), "F3"},
        {DApplication::translate("Shortcuts", "Delete note"), "Delete"},
        {DApplication::translate("Shortcuts", "Play/Pause"), "Space"},
        {DApplication::translate("Shortcuts", "Record voice"), "Ctrl+R"},
        {DApplication::translate("Shortcuts", "Voice to Text"), "Ctrl+W"},
        {DApplication::translate("Shortcuts", "Save as MP3"), "Ctrl+P"},
        {DApplication::translate("Shortcuts", "Save as TXT"), "Ctrl+S"},
        {DApplication::translate("Shortcuts", "Save recordings"), "Ctrl+Y"},
    };

    QJsonObject noteJsonGroup;
    noteJsonGroup.insert("groupName", DApplication::translate("ShortcutsGroups", "Notes"));
    QJsonArray noteJsonItems;

    for (QMap<QString, QString>::iterator it = shortcutNoteKeymap.begin();
            it != shortcutNoteKeymap.end(); it++) {
        QJsonObject jsonItem;
        jsonItem.insert("name", it.key());
        jsonItem.insert("value", it.value().replace("Meta", "Super"));
        noteJsonItems.append(jsonItem);
    }

    noteJsonGroup.insert("groupItems", noteJsonItems);
    jsonGroups.append(noteJsonGroup);
    //******************************Edit***************************************************
    QMap<QString, QString> shortcutEditKeymap = {
        //Edit
        {DApplication::translate("Shortcuts", "Select all"), "Ctrl+A"},
        {DApplication::translate("Shortcuts", "Copy"), "Ctrl+C"},
        {DApplication::translate("Shortcuts", "Cut"), "Ctrl+X"},
        {DApplication::translate("Shortcuts", "Paste"), "Ctrl+V"},
        {DApplication::translate("Shortcuts", "Delete"), "Delete"},
    };

    QJsonObject editJsonGroup;
    editJsonGroup.insert("groupName", DApplication::translate("ShortcutsGroups", "Edit"));
    QJsonArray editJsonItems;

    for (QMap<QString, QString>::iterator it = shortcutEditKeymap.begin();
            it != shortcutEditKeymap.end(); it++) {
        QJsonObject jsonItem;
        jsonItem.insert("name", it.key());
        jsonItem.insert("value", it.value().replace("Meta", "Super"));
        editJsonItems.append(jsonItem);
    }

    editJsonGroup.insert("groupItems", editJsonItems);
    jsonGroups.append(editJsonGroup);
    //******************************Setting************************************************
    QMap<QString, QString> shortcutSettingKeymap = {
        //Setting
        //        {DApplication::translate("Shortcuts","Close window"),         "Alt+F4"},
        //        {DApplication::translate("Shortcuts","Resize window"),        "Ctrl+Alt+F"},
        //        {DApplication::translate("Shortcuts","Find"),                 "Ctrl+F"},
        {DApplication::translate("Shortcuts", "Help"), "F1"},
        {DApplication::translate("Shortcuts", "Display shortcuts"), "Ctrl+Shift+?"},
    };

    QJsonObject settingJsonGroup;
    settingJsonGroup.insert("groupName", DApplication::translate("ShortcutsGroups", "Settings"));
    QJsonArray settingJsonItems;

    for (QMap<QString, QString>::iterator it = shortcutSettingKeymap.begin();
            it != shortcutSettingKeymap.end(); it++) {
        QJsonObject jsonItem;
        jsonItem.insert("name", it.key());
        jsonItem.insert("value", it.value().replace("Meta", "Super"));
        settingJsonItems.append(jsonItem);
    }

    settingJsonGroup.insert("groupItems", settingJsonItems);
    jsonGroups.append(settingJsonGroup);

    shortcutObj.insert("shortcut", jsonGroups);

    QJsonDocument doc(shortcutObj);

    QStringList shortcutString;
    QString param1 = "-j=" + QString(doc.toJson().data());
    QString param2 = "-p=" + QString::number(pos.x()) + "," + QString::number(pos.y());
    shortcutString << param1 << param2;

    QProcess *shortcutViewProcess = new QProcess();
    shortcutViewProcess->startDetached("deepin-shortcut-viewer", shortcutString);

    connect(shortcutViewProcess, SIGNAL(finished(int)), shortcutViewProcess, SLOT(deleteLater()));
}

/**
 * @brief VNoteMainWindow::closeEvent
 * @param event
 */
void VNoteMainWindow::closeEvent(QCloseEvent *event)
{
    if (checkIfNeedExit()) {
        if (stateOperation->isRecording()) {
            stateOperation->operState(OpsStateInterface::StateAppQuit, true);
            m_recordBar->stopRecord();
            event->ignore();
        } else {
            release();
            exit(0);
        }
    } else {
        event->ignore();
    }
}

/**
 * @brief VNoteMainWindow::resizeEvent
 * @param event
 */
void VNoteMainWindow::resizeEvent(QResizeEvent *event)
{
    if (m_asrErrMeassage && m_asrErrMeassage->isVisible()) {
        int xPos = (m_centerWidget->width() - m_asrErrMeassage->width()) / 2;
        int yPos = m_centerWidget->height() - m_asrErrMeassage->height() - 5;
        m_asrErrMeassage->move(xPos, yPos);
    }

    if (m_pDeviceExceptionMsg && m_pDeviceExceptionMsg->isVisible()) {
        int xPos = (m_centerWidget->width() - m_pDeviceExceptionMsg->width()) / 2;
        int yPos = m_centerWidget->height() - m_pDeviceExceptionMsg->height() - 5;
        m_pDeviceExceptionMsg->move(xPos, yPos);
    }

    DMainWindow::resizeEvent(event);
}

/**
 * @brief VNoteMainWindow::keyPressEvent
 * @param event
 */
void VNoteMainWindow::keyPressEvent(QKeyEvent *event)
{
    DMainWindow::keyPressEvent(event);
}

/**
 * @brief VNoteMainWindow::checkIfNeedExit
 * @return true 关闭应用
 */
bool VNoteMainWindow::checkIfNeedExit()
{
    QScopedPointer<VNoteMessageDialog> pspMessageDialg;

    bool bNeedExit = true;

    //Is audio converting to text
    if (stateOperation->isVoice2Text()) {
        pspMessageDialg.reset(new VNoteMessageDialog(
                                  VNoteMessageDialog::AborteAsr,
                                  this));
    } else if (stateOperation->isRecording()) { //Is recording
        pspMessageDialg.reset(new VNoteMessageDialog(
                                  VNoteMessageDialog::AbortRecord,
                                  this));
    }

    if (!pspMessageDialg.isNull()) {
        //Use cancel exit.
        if (QDialog::Rejected == pspMessageDialg->exec()) {
            bNeedExit = false;
        }
    }

    return bNeedExit;
}

/**
 * @brief VNoteMainWindow::onVNoteChange
 * @param previous
 */
void VNoteMainWindow::onVNoteChange(const QModelIndex &previous)
{
    Q_UNUSED(previous);
    if (m_asrErrMeassage) {
        m_asrErrMeassage->setVisible(false);
    }

    QModelIndex index = m_middleView->currentIndex();
    VNoteItem *data = static_cast<VNoteItem *>(StandardItemCommon::getStandardItemData(index));

    if (data == nullptr || stateOperation->isSearching()) {
        m_recordBar->setVisible(false);
    } else {
        m_recordBar->setVisible(true);
    }
#if 0
    QScrollBar *bar = m_rightViewScrollArea->verticalScrollBar();
    bar->setValue(bar->minimum());

    m_rightView->initData(data, m_searchKey, m_rightViewHasFouse);
#endif
    m_webView->initData(data, m_searchKey, m_rightViewHasFouse);
    m_rightViewHasFouse = false;
}

/**
 * @brief VNoteMainWindow::onMenuAction
 * @param action
 */
void VNoteMainWindow::onMenuAction(QAction *action)
{
    ActionManager::ActionKind kind = ActionManager::Instance()->getActionKind(action);
    switch (kind) {
    case ActionManager::NotebookRename:
        editNotepad();
        break;
    case ActionManager::NotebookDelete: {
        VNoteMessageDialog confirmDialog(VNoteMessageDialog::DeleteFolder, this);
        connect(&confirmDialog, &VNoteMessageDialog::accepted, this, [this]() {
            delNotepad();
        });

        confirmDialog.exec();
    } break;
    case ActionManager::NotebookAddNew:
        addNote();
        break;
    case ActionManager::NoteDelete: {
        VNoteMessageDialog confirmDialog(VNoteMessageDialog::DeleteNote, this);
        connect(&confirmDialog, &VNoteMessageDialog::accepted, this, [this]() {
            delNote();
        });

        confirmDialog.exec();
    } break;
    case ActionManager::NoteAddNew:
        addNote();
        break;
    case ActionManager::NoteRename:
        editNote();
        break;
    case ActionManager::DetailDelete: {
        VNoteBlock *data = JsContent::instance()->getCurrentBlock();
        if (data) {
            if (data->getType() == VNoteBlock::Voice) {
                m_webView->deleteVoice(data->blockid);
            } else {
                m_webView->triggerPageAction(QWebEnginePage::Cut);
            }
        }

    } break;
    case ActionManager::NoteSaveText: {
        m_middleView->saveAsText();
    } break;
    case ActionManager::NoteSaveVoice: {
        m_middleView->saveRecords();
    } break;
    case ActionManager::DetailVoice2Text:
        onA2TStart();
        break;
    case ActionManager::DetailSelectAll: {
        VNoteBlock *data = JsContent::instance()->getCurrentBlock();
        if (data && data->getType() == VNoteBlock::Text) {
            m_webView->triggerPageAction(QWebEnginePage::SelectAll);
        }
    }
    break;
    case ActionManager::DetailCopy: {
        VNoteBlock *data = JsContent::instance()->getCurrentBlock();
        if (data && data->getType() == VNoteBlock::Text) {
            m_webView->triggerPageAction(QWebEnginePage::Copy);
        }
    }
    break;
    case ActionManager::DetailPaste: {
        VNoteBlock *data = JsContent::instance()->getCurrentBlock();
        if (data && data->getType() == VNoteBlock::Text) {
            m_webView->triggerPageAction(QWebEnginePage::Paste);
        }
    }
    break;
    case ActionManager::DetailCut: {
        VNoteBlock *data = JsContent::instance()->getCurrentBlock();
        if (data && data->getType() == VNoteBlock::Text) {
            m_webView->triggerPageAction(QWebEnginePage::Cut);
        }
    }
    break;
//    case ActionManager::DetailVoiceSave:
//        m_rightView->saveMp3();
//        break;
    case ActionManager::DetailText2Speech:
        VTextSpeechAndTrManager::onTextToSpeech();
        break;
    case ActionManager::DetailStopreading:
        VTextSpeechAndTrManager::onStopTextToSpeech();
        break;
    case ActionManager::DetailSpeech2Text:
        VTextSpeechAndTrManager::onSpeechToText();
        break;
    case ActionManager::DetailTranslate:
        VTextSpeechAndTrManager::onTextTranslate();
        break;
    default:
        break;
    }
}

/**
 * @brief VNoteMainWindow::onMenuAbout2Show
 */
void VNoteMainWindow::onMenuAbout2Show()
{
    //TODO:
    //    Add context menu item disable/enable code here
    //Eg:
    //ActionManager::Instance()->enableAction(ActionManager::NoteAddNew, false);
    DMenu *menu = static_cast<DMenu *>(sender());

    if (menu == ActionManager::Instance()->noteContextMenu()) {
        ActionManager::Instance()->resetCtxMenu(ActionManager::MenuType::NoteCtxMenu);

        if (stateOperation->isPlaying()
                || stateOperation->isRecording()
                || stateOperation->isVoice2Text()) {
            ActionManager::Instance()->enableAction(ActionManager::NoteAddNew, false);
            ActionManager::Instance()->enableAction(ActionManager::NoteDelete, false);

            if (stateOperation->isRecording()) {
                ActionManager::Instance()->enableAction(ActionManager::NoteSaveVoice, false);
            }
        } else if (stateOperation->isSearching()) {
            ActionManager::Instance()->enableAction(ActionManager::NoteAddNew, false);
        }

        //Disable SaveText if note have no text
        //Disable SaveVoice if note have no voice.
        VNoteItem *currNoteData = m_middleView->getCurrVNotedata();

        if (nullptr != currNoteData) {
            if (!currNoteData->haveText()) {
                ActionManager::Instance()->enableAction(ActionManager::NoteSaveText, false);
            }

            if (!currNoteData->haveVoice()) {
                ActionManager::Instance()->enableAction(ActionManager::NoteSaveVoice, false);
            }
        }
    } else if (menu == ActionManager::Instance()->notebookContextMenu()) {
        ActionManager::Instance()->resetCtxMenu(ActionManager::MenuType::NotebookCtxMenu);

        if (stateOperation->isPlaying()
                || stateOperation->isRecording()
                || stateOperation->isVoice2Text()) {
            ActionManager::Instance()->enableAction(ActionManager::NotebookAddNew, false);
            ActionManager::Instance()->enableAction(ActionManager::NotebookDelete, false);
        }
    }
}

/**
 * @brief VNoteMainWindow::loadNotepads
 * @return 记事本数量
 */
int VNoteMainWindow::loadNotepads()
{
    VNOTE_FOLDERS_MAP *folders = VNoteDataManager::instance()->getNoteFolders();

    int folderCount = 0;

    if (folders) {
        folders->lock.lockForRead();

        folderCount = folders->folders.size();

        for (auto it : folders->folders) {
            m_leftView->appendFolder(it);
        }

        folders->lock.unlock();
    }

    return folderCount;
}

/**
 * @brief VNoteMainWindow::addNotepad
 */
void VNoteMainWindow::addNotepad()
{
    VNoteFolder itemData;
    VNoteFolderOper folderOper;
    itemData.name = folderOper.getDefaultFolderName();

    VNoteFolder *newFolder = folderOper.addFolder(itemData);

    if (newFolder) {
        //Switch to note view
        switchWidget(WndNoteShow);
        m_leftView->addFolder(newFolder);
        addNote();
    }
}

/**
 * @brief VNoteMainWindow::delNotepad
 */
void VNoteMainWindow::delNotepad()
{
    VNoteFolder *data = m_leftView->removeFolder();

    //m_rightView->removeCacheWidget(data);

    VNoteFolderOper folderOper(data);

    folderOper.deleteVNoteFolder(data);

    if (m_leftView->folderCount() == 0) {
        switchWidget(WndHomePage);
    }
}

/**
 * @brief VNoteMainWindow::editNotepad
 */
void VNoteMainWindow::editNotepad()
{
    m_leftView->editFolder();
}

/**
 * @brief VNoteMainWindow::addNote
 */
void VNoteMainWindow::addNote()
{
    qint64 id = m_middleView->getCurrentId();
    if (id != -1) {
        m_rightViewHasFouse = true;
        VNoteItem tmpNote;
        tmpNote.folderId = id;
        tmpNote.noteType = VNoteItem::VNT_Text;

        //Add default text block
        //Comment:
        //    ptrBlock will be released when the data set is destroyed,
        //so don't need release it here.
        VNoteBlock *ptrBlock = tmpNote.newBlock(VNoteBlock::Text);
        tmpNote.addBlock(ptrBlock);

        VNoteItemOper noteOper;
        //Get default note name in the folder
        tmpNote.noteTitle = noteOper.getDefaultNoteName(tmpNote.folderId);

        VNoteItem *newNote = noteOper.addNote(tmpNote);

        //Refresh the notes count of folder
        m_leftView->update(m_leftView->currentIndex());
        //m_rightView->saveNote();
        m_middleView->addRowAtHead(newNote);
    }
}

/**
 * @brief VNoteMainWindow::editNote
 */
void VNoteMainWindow::editNote()
{
    m_middleView->editNote();
}

/**
 * @brief VNoteMainWindow::delNote
 */
void VNoteMainWindow::delNote()
{
    VNoteItem *noteData = m_middleView->deleteCurrentRow();

    if (noteData) {
        //m_rightView->removeCacheWidget(noteData);
        VNoteItemOper noteOper(noteData);
        noteOper.deleteNote();

        //Refresh the middle view
        if (m_middleView->rowCount() <= 0 && stateOperation->isSearching()) {
            m_middleView->setVisibleEmptySearch(true);
        }

        //Refresh the notes count of folder
        m_leftView->update(m_leftView->currentIndex());
    }
}

/**
 * @brief VNoteMainWindow::loadNotes
 * @param folder
 * @return 记事项数量
 */
int VNoteMainWindow::loadNotes(VNoteFolder *folder)
{
    m_middleView->clearAll();
    m_middleView->setVisibleEmptySearch(false);
    if (folder) {
        m_middleView->setCurrentId(folder->id);
        VNoteItemOper noteOper;
        VNOTE_ITEMS_MAP *notes = noteOper.getFolderNotes(folder->id);
        if (notes) {
            notes->lock.lockForRead();
            for (auto it : notes->folderNotes) {
                m_middleView->appendRow(it);
            }
            notes->lock.unlock();

            //Sort the view & set focus on first item
            m_middleView->onNoteChanged();
            m_middleView->setCurrentIndex(0);
        }
    } else {
        m_middleView->setCurrentId(-1);
    }
    return m_middleView->rowCount();
}

/**
 * @brief VNoteMainWindow::loadSearchNotes
 * @param key
 * @return 记事项数量
 */
int VNoteMainWindow::loadSearchNotes(const QString &key)
{
    m_middleView->clearAll();
    m_middleView->setSearchKey(key);
    VNOTE_ALL_NOTES_MAP *noteAll = VNoteDataManager::instance()->getAllNotesInFolder();
    if (noteAll) {
        noteAll->lock.lockForRead();
        for (auto &foldeNotes : noteAll->notes) {
            for (auto note : foldeNotes->folderNotes) {
                if (note->search(key)) {
                    m_middleView->appendRow(note);
                }
            }
        }
        noteAll->lock.unlock();
        if (m_middleView->rowCount() == 0) {
            m_middleView->setVisibleEmptySearch(true);
            //m_rightView->initData(nullptr, m_searchKey);
            m_recordBar->setVisible(false);
        } else {
            m_middleView->setVisibleEmptySearch(false);
            m_middleView->setCurrentIndex(0);
        }
    }
    return m_middleView->rowCount();
}

/**
 * @brief VNoteMainWindow::onRightViewVoicePlay
 * @param voiceData
 */
void VNoteMainWindow::onRightViewVoicePlay(VNVoiceBlock *voiceData)
{
    setSpecialStatus(PlayVoiceStart);
    m_recordBar->playVoice(voiceData);
}

/**
 * @brief VNoteMainWindow::onRightViewVoicePause
 * @param voiceData
 */
void VNoteMainWindow::onRightViewVoicePause(VNVoiceBlock *voiceData)
{
    m_recordBar->pauseVoice(voiceData);
}

/**
 * @brief VNoteMainWindow::onPlayPlugVoicePlay
 * @param voiceData
 */
void VNoteMainWindow::onPlayPlugVoicePlay(VNVoiceBlock *voiceData)
{
//    VoiceNoteItem *voiceItem = m_rightView->getCurVoicePlay();
//    if (voiceItem && voiceItem->getNoteBlock() == voiceData) {
//        voiceItem->showPauseBtn();
//    }
    emit JsContent::instance()->switchPlayBtn(1, QString::number(reinterpret_cast<qint64>(voiceData)));
}

/**
 * @brief VNoteMainWindow::onPlayPlugVoicePause
 * @param voiceData
 */
void VNoteMainWindow::onPlayPlugVoicePause(VNVoiceBlock *voiceData)
{
//    VoiceNoteItem *voiceItem = m_rightView->getCurVoicePlay();
//    if (voiceItem && voiceItem->getNoteBlock() == voiceData) {
//        voiceItem->showPlayBtn();
//    }
    emit JsContent::instance()->switchPlayBtn(0, QString::number(reinterpret_cast<qint64>(voiceData)));
}

/**
 * @brief VNoteMainWindow::onPlayPlugVoiceStop
 * @param voiceData
 */
void VNoteMainWindow::onPlayPlugVoiceStop(VNVoiceBlock *voiceData)
{
//    VoiceNoteItem *voiceItem = m_rightView->getCurVoicePlay();
//    if (voiceItem && voiceItem->getNoteBlock() == voiceData) {
//        voiceItem->showPlayBtn();
//    }
    emit JsContent::instance()->switchPlayBtn(0, QString::number(reinterpret_cast<qint64>(voiceData)));
    setSpecialStatus(PlayVoiceEnd);
//    m_rightView->setCurVoicePlay(nullptr);
//    m_rightView->setFocus();

}

/**
 * @brief VNoteMainWindow::onNewNotebook
 */
void VNoteMainWindow::onNewNotebook()
{
    static struct timeval curret = {0, 0};
    static struct timeval lastPress = {0, 0};

    gettimeofday(&curret, nullptr);

    if (TM(lastPress, curret) > MIN_STKEY_RESP_TIME) {
        addNotepad();

        UPT(lastPress, curret);
    }
}

/**
 * @brief VNoteMainWindow::onNewNote
 */
void VNoteMainWindow::onNewNote()
{
    static struct timeval curret = {0, 0};
    static struct timeval lastPress = {0, 0};

    gettimeofday(&curret, nullptr);

    if (TM(lastPress, curret) > MIN_STKEY_RESP_TIME) {
        addNote();

        UPT(lastPress, curret);
    }
}

/**
 * @brief VNoteMainWindow::canDoShortcutAction
 * @return true 可以使用快捷键
 */
bool VNoteMainWindow::canDoShortcutAction() const
{
    return (m_stackedWidget->currentIndex() == WndNoteShow);
}

/**
 * @brief VNoteMainWindow::setSpecialStatus
 * @param status
 */
void VNoteMainWindow::setSpecialStatus(SpecialStatus status)
{
    switch (status) {
    case SearchStart:
        if (!stateOperation->isSearching()) {
            stateOperation->operState(OpsStateInterface::StateSearching, true);
            m_leftView->clearSelection();
            m_leftView->setEnabled(false);
            m_addNotepadBtn->setVisible(false);
            m_addNoteBtn->setVisible(false);
        }
        break;
    case SearchEnd:
        if (stateOperation->isSearching()) {
            m_searchKey = "";
            m_middleView->setSearchKey(m_searchKey);
            m_leftView->setEnabled(true);
            m_addNotepadBtn->setVisible(true);
            m_addNoteBtn->setVisible(true);
            m_noteSearchEdit->lineEdit()->setFocus();
            stateOperation->operState(OpsStateInterface::StateSearching, false);
            onVNoteFolderChange(m_leftView->restoreNotepadItem(), QModelIndex());
        }
        break;
    case PlayVoiceStart:
        if (stateOperation->isSearching()) {
            m_recordBar->setVisible(true);
        }
        stateOperation->operState(OpsStateInterface::StatePlaying, true);
        m_noteSearchEdit->setEnabled(false);
        m_leftView->setOnlyCurItemMenuEnable(true);
        m_addNotepadBtn->setEnabled(false);
        m_middleView->setOnlyCurItemMenuEnable(true);
        m_addNoteBtn->setDisabled(true);
        break;
    case PlayVoiceEnd:
        if (stateOperation->isSearching()) {
            m_recordBar->setVisible(false);
        }
        if (!stateOperation->isVoice2Text()) {
            m_noteSearchEdit->setEnabled(true);
            m_leftView->setOnlyCurItemMenuEnable(false);
            m_addNotepadBtn->setEnabled(true);
            m_middleView->setOnlyCurItemMenuEnable(false);
            m_addNoteBtn->setDisabled(false);
        }
        stateOperation->operState(OpsStateInterface::StatePlaying, false);
        break;
    case RecordStart:
        stateOperation->operState(OpsStateInterface::StateRecording, true);
        m_noteSearchEdit->setEnabled(false);
        m_leftView->setOnlyCurItemMenuEnable(true);
        m_leftView->closeMenu();
        m_middleView->closeMenu();
        m_addNotepadBtn->setEnabled(false);
        m_middleView->setOnlyCurItemMenuEnable(true);
        //m_rightView->setEnablePlayBtn(false);
        m_addNoteBtn->setDisabled(true);
        break;
    case RecordEnd:
        if (!stateOperation->isVoice2Text()) {
            m_noteSearchEdit->setEnabled(true);
            m_leftView->setOnlyCurItemMenuEnable(false);
            m_addNotepadBtn->setEnabled(true);
            m_middleView->setOnlyCurItemMenuEnable(false);
            m_addNoteBtn->setDisabled(false);
        }
        //m_rightView->setEnablePlayBtn(true);
        stateOperation->operState(OpsStateInterface::StateRecording, false);
        break;
    case VoiceToTextStart:
        stateOperation->operState(OpsStateInterface::StateVoice2Text, true);
        m_noteSearchEdit->setEnabled(false);
        m_leftView->setOnlyCurItemMenuEnable(true);
        m_addNotepadBtn->setEnabled(false);
        m_middleView->setOnlyCurItemMenuEnable(true);
        m_addNoteBtn->setDisabled(true);
        m_leftView->closeMenu();
        m_middleView->closeMenu();
        //m_rightView->closeMenu();
        break;
    case VoiceToTextEnd:
        if (!stateOperation->isRecording() && !stateOperation->isPlaying()) {
            m_noteSearchEdit->setEnabled(true);
            m_leftView->setOnlyCurItemMenuEnable(false);
            m_addNotepadBtn->setEnabled(true);
            m_middleView->setOnlyCurItemMenuEnable(false);
            m_addNoteBtn->setDisabled(false);
        }
        stateOperation->operState(OpsStateInterface::StateVoice2Text, false);
        break;
    default:
        break;
    }
}

/**
 * @brief VNoteMainWindow::initAsrErrMessage
 */
void VNoteMainWindow::initAsrErrMessage()
{
    m_asrErrMeassage = new DFloatingMessage(DFloatingMessage::ResidentType,
                                            m_centerWidget);
    QString iconPath = STAND_ICON_PAHT;
    iconPath.append("warning.svg");
    m_asrErrMeassage->setIcon(QIcon(iconPath));
    m_asrAgainBtn = new DPushButton(m_asrErrMeassage);
    m_asrAgainBtn->setText(DApplication::translate(
                               "VNoteErrorMessage",
                               "Try Again"));
    m_asrAgainBtn->adjustSize();
    connect(m_asrAgainBtn, &DPushButton::clicked,
            this, &VNoteMainWindow::onA2TStartAgain);
    DWidget *m_widget = new DWidget(m_asrErrMeassage);
    QHBoxLayout *m_layout = new QHBoxLayout();
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->addStretch();
    m_layout->addWidget(m_asrAgainBtn);
    m_widget->setLayout(m_layout);
    m_asrErrMeassage->setWidget(m_widget);
    m_asrErrMeassage->setVisible(false);

}

/**
 * @brief VNoteMainWindow::initDeviceExceptionErrMessage
 */
void VNoteMainWindow::initDeviceExceptionErrMessage()
{
    m_pDeviceExceptionMsg = new DFloatingMessage(DFloatingMessage::ResidentType,
                                                 m_centerWidget);
    QString iconPath = STAND_ICON_PAHT;
    iconPath.append("warning.svg");
    m_pDeviceExceptionMsg->setIcon(QIcon(iconPath));
    m_pDeviceExceptionMsg->setMessage(
        DApplication::translate(
            "VNoteRecordBar",
            "Your audio recording device does not work."));
    m_pDeviceExceptionMsg->setVisible(false);
}

/**
 * @brief VNoteMainWindow::showAsrErrMessage
 * @param strMessage
 */
void VNoteMainWindow::showAsrErrMessage(const QString &strMessage)
{
    if (m_asrErrMeassage == nullptr) {
        initAsrErrMessage();
    }

    m_asrErrMeassage->setMessage(strMessage);
    m_asrErrMeassage->setVisible(true);
    m_asrErrMeassage->setMinimumWidth(520);
    m_asrErrMeassage->setMinimumHeight(60);
    m_asrErrMeassage->setMaximumWidth(m_centerWidget->width());
    m_asrErrMeassage->adjustSize();

    int xPos = (m_centerWidget->width() - m_asrErrMeassage->width()) / 2;
    int yPos = m_centerWidget->height() - m_asrErrMeassage->height() - 5;

    m_asrErrMeassage->move(xPos, yPos);
}

/**
 * @brief VNoteMainWindow::showDeviceExceptionErrMessage
 */
void VNoteMainWindow::showDeviceExceptionErrMessage()
{
    if (m_pDeviceExceptionMsg == nullptr) {
        initDeviceExceptionErrMessage();
    }

    m_pDeviceExceptionMsg->setVisible(true);
    m_pDeviceExceptionMsg->setMinimumWidth(520);
    m_pDeviceExceptionMsg->setMinimumHeight(65);
    m_pDeviceExceptionMsg->setMaximumWidth(m_centerWidget->width());
    m_pDeviceExceptionMsg->adjustSize();

    int xPos = (m_centerWidget->width() - m_pDeviceExceptionMsg->width()) / 2;
    int yPos = m_centerWidget->height() - m_pDeviceExceptionMsg->height() - 5;

    m_pDeviceExceptionMsg->move(xPos, yPos);
}

/**
 * @brief VNoteMainWindow::closeDeviceExceptionErrMessage
 */
void VNoteMainWindow::closeDeviceExceptionErrMessage()
{
    if (m_pDeviceExceptionMsg) {
        m_pDeviceExceptionMsg->setVisible(false);
    }
}

/**
 * @brief VNoteMainWindow::onSystemDown
 * @param active
 */
void VNoteMainWindow::onSystemDown(bool active)
{
    qInfo() << "System going down...";

    if (active) {
        if (stateOperation->isRecording()) {
            m_recordBar->stopRecord();

            qInfo() << "System going down when recording, cancel it.";
        }

        releaseHaltLock();
    }
}

/**
 * @brief VNoteMainWindow::onCursorChange
 * @param height
 * @param mouseMove true 鼠标拖动
 */
void VNoteMainWindow::onCursorChange(int height, bool mouseMove)
{
    QScrollBar *bar = m_rightViewScrollArea->verticalScrollBar();
    if (height > bar->value() + m_rightViewScrollArea->height()) {
        int value = height - m_rightViewScrollArea->height();
        if (!mouseMove) {
            if (value > bar->maximum()) {
                bar->setMaximum(value);
            }
        }
        bar->setValue(value);
    }
    int value = bar->value();
    if (value > height) {
        bar->setValue(height);
    }
}

/**
 * @brief VNoteMainWindow::switchWidget
 * @param type
 */
void VNoteMainWindow::switchWidget(WindowType type)
{
    //bool searchEnable = type == WndNoteShow ? true : false;
    m_noteSearchEdit->setEnabled(false);
    m_stackedWidget->setCurrentIndex(type);
}

/**
 * @brief VNoteMainWindow::release
 */
void VNoteMainWindow::release()
{
    //Save main window size
    if (!isMaximized()) {
        setting::instance()->setOption(VNOTE_MAINWND_SZ_KEY, saveGeometry());
    }

    VTextSpeechAndTrManager::onStopTextToSpeech();
    // m_rightView->saveNote();

    QScopedPointer<VNoteA2TManager> releaseA2TManger(m_a2tManager);
    if (stateOperation->isVoice2Text()) {
        releaseA2TManger->stopAsr();
    }

    VNoteDbManager::instance()->getVNoteDb().close();
}

/**
 * @brief VNoteMainWindow::onShowPrivacy
 */
void VNoteMainWindow::onShowPrivacy()
{
    QString url = "";
    QLocale locale;
    QLocale::Country country = locale.country();
    bool isCommunityEdition = DSysInfo::isCommunityEdition();
    if (country == QLocale::China) {
        if (isCommunityEdition) {
            url = "https://www.uniontech.com/agreement/deepin-privacy-cn";
        } else {
            url = "https://www.uniontech.com/agreement/privacy-cn";
        }
    } else {
        if (isCommunityEdition) {
            url = "https://www.uniontech.com/agreement/deepin-privacy-en";
        } else {
            url = "https://www.uniontech.com/agreement/privacy-en";
        }
    }
    QDesktopServices::openUrl(url);
}

/**
 * @brief VNoteMainWindow::onShowSettingDialog
 */
void VNoteMainWindow::onShowSettingDialog()
{
    DSettingsDialog dialog(this);
    dialog.updateSettings("Setting", setting::instance()->getSetting());
    dialog.setResetVisible(false);
    dialog.exec();
}

/**
 * @brief VNoteMainWindow::initMenuExtension
 */
void VNoteMainWindow::initMenuExtension()
{
    m_menuExtension = new DMenu(this);
    QAction *setting = new QAction(DApplication::translate("TitleBar", "Settings"), m_menuExtension);
    QAction *privacy = new QAction(DApplication::translate("TitleBar", "Privacy Policy"), m_menuExtension);
    m_menuExtension->addAction(setting);
    m_menuExtension->addAction(privacy);
    m_menuExtension->addSeparator();
    connect(privacy, &QAction::triggered, this, &VNoteMainWindow::onShowPrivacy);
    connect(setting, &QAction::triggered, this, &VNoteMainWindow::onShowSettingDialog);
}
