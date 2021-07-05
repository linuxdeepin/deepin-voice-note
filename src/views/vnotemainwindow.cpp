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
#include "widgets/vnotemultiplechoiceoptionwidget.h"

#include "common/utils.h"
#include "common/actionmanager.h"
#include "common/setting.h"
#include "common/performancemonitor.h"

#include "db/vnotefolderoper.h"
#include "db/vnoteitemoper.h"
#include "db/vnotedbmanager.h"
#include "db/vnotesaferoper.h"

#include "dbus/dbuslogin1manager.h"

#include "dialog/vnotemessagedialog.h"
#include "views/vnoterecordbar.h"
#include "widgets/vnoteiconbutton.h"
#include "task/vnmainwnddelayinittask.h"

#ifdef IMPORT_OLD_VERSION_DATA
#include "importolddata/upgradeview.h"
#include "importolddata/upgradedbutil.h"
#endif

#include "vnoteapplication.h"

#include <DApplication>
#include <DToolButton>
#include <DApplicationHelper>
#include <DFontSizeManager>
#include <DLog>
#include <DStatusBar>
#include <DTitlebar>
#include <DSettingsDialog>
#include <DSysInfo>

#include <QScrollBar>
#include <QLocale>
#include <QDesktopServices>

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
    //delayInitTasks();
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

    connect(m_middleView, &MiddleView::requestRefresh,
            m_rightView, &RightView::refreshVoiceCreateTime);

    connect(m_rightView, &RightView::contentChanged,
            m_middleView, &MiddleView::onNoteChanged);

    connect(m_rightView, &RightView::sigVoicePlay,
            this, &VNoteMainWindow::onRightViewVoicePlay);
    connect(m_rightView, &RightView::sigVoicePause,
            this, &VNoteMainWindow::onRightViewVoicePause);
    connect(m_rightView, &RightView::sigCursorChange,
            this, &VNoteMainWindow::onCursorChange);

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
    //处理笔记拖拽释放
    connect(m_leftView, &LeftView::dropNotesEnd, this, &VNoteMainWindow::onDropNote);
    //处理详情页刷新请求
    connect(m_middleView, &MiddleView::requestChangeRightView, this, &VNoteMainWindow::changeRightView);
    //处理详情页多选操作
    connect(m_multipleSelectWidget, &VnoteMultipleChoiceOptionWidget::requestMultipleOption, this, &VNoteMainWindow::handleMultipleOption);
}

/**
 * @brief VNoteMainWindow::changeRightView
 */
//刷新详情页显示
void VNoteMainWindow::changeRightView(bool isMultiple)
{
    m_multipleSelectWidget->setNoteNumber(m_middleView->getSelectedCount());
    if (isMultiple) {
        if (m_rightView->getIsNormalView()) {
            m_stackedRightMainWidget->setCurrentWidget(m_multipleSelectWidget);
            m_rightView->setIsNormalView(false);
        }
        //设置按钮是否置灰
        bool moveButtonEnable = true;
        if (stateOperation->isVoice2Text()
            || stateOperation->isSearching()
            || 1 == m_leftView->folderCount()) {
            moveButtonEnable = false;
        }
        m_multipleSelectWidget->enableButtons(m_middleView->haveText(), m_middleView->haveVoice(), moveButtonEnable);
    } else {
        if (!m_rightView->getIsNormalView()) {
            m_stackedRightMainWidget->setCurrentWidget(m_rightViewHolder);
            m_rightView->setIsNormalView(true);
        }
    }
}

/**
 * @brief VNoteMainWindow::initShortcuts
 */
void VNoteMainWindow::initShortcuts()
{
    m_stEsc.reset(new QShortcut(this));
    m_stEsc->setKey(Qt::Key_Escape);
    m_stEsc->setContext(Qt::WidgetWithChildrenShortcut);
    m_stEsc->setAutoRepeat(false);

    connect(m_stEsc.get(), &QShortcut::activated, this, [=] {
        setTitleBarTabFocus();
    });

    m_stPopupMenu.reset(new QShortcut(this));
    m_stPopupMenu->setKey(Qt::ALT + Qt::Key_M);
    m_stPopupMenu->setContext(Qt::ApplicationShortcut);
    m_stPopupMenu->setAutoRepeat(false);

    connect(m_stPopupMenu.get(), &QShortcut::activated, this, [this] {
        if (m_leftView->hasFocus()) {
            m_leftView->popupMenu();
        } else if (m_middleView->hasFocus()) {
            m_middleView->popupMenu();
        } else if (m_noteSearchEdit->lineEdit()->hasFocus()) {
            if (!m_showSearchEditMenu) {
                QPoint pos(m_noteSearchEdit->pos());
                QContextMenuEvent eve(QContextMenuEvent::Reason::Keyboard, pos, m_noteSearchEdit->mapToGlobal(pos));
                m_showSearchEditMenu = QApplication::sendEvent(m_noteSearchEdit->lineEdit(), &eve);
            }
        } else {
            m_rightView->popupMenu();
        }
    });

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
              || stateOperation->isSearching())) {
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
        //当前记事本是否正在编辑
        bool isEditing = m_leftView->isPersistentEditorOpen(m_leftView->currentIndex());
        //如果已在编辑状态，取消操作，解决重复快捷键警告信息
        if (canDoShortcutAction()) {
            if (!stateOperation->isSearching() && !isEditing) {
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
                 || stateOperation->isSearching())) {
            onNewNote();
        }
    });

    //Rename note
    m_stRemNote.reset(new QShortcut(this));
    m_stRemNote->setKey(Qt::Key_F3);
    m_stRemNote->setContext(Qt::ApplicationShortcut);
    m_stRemNote->setAutoRepeat(false);

    connect(m_stRemNote.get(), &QShortcut::activated, this, [this] {
        //当前笔记是否正在编辑
        bool isEditing = m_middleView->isPersistentEditorOpen(m_middleView->currentIndex());
        //如果已在编辑状态，取消操作，解决重复快捷键警告信息
        if (canDoShortcutAction() && !isEditing) {
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
        if (canDoShortcutAction()) {
            m_recordBar->playOrPauseVoice();
        }
    });

    //Add new recording
    m_stRecording.reset(new QShortcut(this));
    m_stRecording->setKey(Qt::CTRL + Qt::Key_R);
    m_stRecording->setContext(Qt::ApplicationShortcut);
    m_stRecording->setAutoRepeat(false);

    connect(m_stRecording.get(), &QShortcut::activated, this, [this] {
        if (canDoShortcutAction()) {
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
        if (canDoShortcutAction()) {
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
        if (canDoShortcutAction()) {
            m_rightView->saveMp3();
        }
    });

    //Save as Text
    m_stSaveAsText.reset(new QShortcut(this));
    m_stSaveAsText->setKey(Qt::CTRL + Qt::Key_S);
    m_stSaveAsText->setContext(Qt::ApplicationShortcut);
    m_stSaveAsText->setAutoRepeat(false);

    connect(m_stSaveAsText.get(), &QShortcut::activated, this, [this] {
        //多选笔记导出文本
        if (canDoShortcutAction()) {
            if (m_middleView->haveText()) {
                m_middleView->saveAsText();
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
        if (canDoShortcutAction()) {
            //Can't save recording when do recording.
            //多选笔记导出语音
            if (!stateOperation->isRecording()) {
                if (m_middleView->haveVoice()) {
                    m_middleView->saveRecords();
                }
            }
        }
    });

    //Notebook/Note/Detial delete key
    m_stDelete.reset(new QShortcut(this));
    m_stDelete->setKey(Qt::Key_Delete);
    m_stDelete->setContext(Qt::ApplicationShortcut);
    m_stDelete->setAutoRepeat(false);

    connect(m_stDelete.get(), &QShortcut::activated, this, &VNoteMainWindow::onDeleteShortcut);

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
    m_noteSearchEdit->lineEdit()->installEventFilter(this);
    titlebar()->addWidget(m_noteSearchEdit);
    DWindowCloseButton *closeBtn = titlebar()->findChild<DWindowCloseButton *>("DTitlebarDWindowCloseButton");
    closeBtn->installEventFilter(this);
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
    leftHolderLayout->setContentsMargins(0, 0, 0, 0);
    m_leftView = new LeftView(m_leftViewHolder);

    //背景色透明，隐藏disable时的背景色
    DPalette pb = DApplicationHelper::instance()->palette(m_leftView->viewport());
    pb.setBrush(DPalette::Base, QColor(0, 0, 0, 0));
    m_leftView->viewport()->setPalette(pb);

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
    m_addNotepadBtn->installEventFilter(this);

    QVBoxLayout *btnLayout = new QVBoxLayout();
    btnLayout->addWidget(m_addNotepadBtn);
    btnLayout->setContentsMargins(10, 0, 10, 10);
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

    m_middleView->installEventFilter(this);
    m_addNoteBtn->installEventFilter(this);
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
    m_stackedRightMainWidget = new QStackedWidget(m_mainWndSpliter);
    m_rightViewHolder = new QWidget(this);

    //初始化多选详情页面
    m_multipleSelectWidget = new VnoteMultipleChoiceOptionWidget(this);
    m_multipleSelectWidget->setBackgroundRole(DPalette::Base);
    m_multipleSelectWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_multipleSelectWidget->setAutoFillBackground(true);

    //添加多选与单选详情页
    m_stackedRightMainWidget->addWidget(m_rightViewHolder);
    m_stackedRightMainWidget->addWidget(m_multipleSelectWidget);

    m_rightViewHolder->setObjectName("rightMainLayoutHolder");
    m_rightViewHolder->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_rightViewHolder->setBackgroundRole(DPalette::Base);
    m_rightViewHolder->setAutoFillBackground(true);

    QVBoxLayout *rightHolderLayout = new QVBoxLayout;
    rightHolderLayout->setSpacing(0);
    rightHolderLayout->setContentsMargins(0, 15, 0, 3);

    m_rightViewScrollArea = new DScrollArea(m_rightViewHolder);
    m_rightView = new RightView(m_rightViewScrollArea);
    m_rightView->installEventFilter(this);
    m_rightViewScrollArea->setLineWidth(0);
    m_rightViewScrollArea->setWidgetResizable(true);
    m_rightViewScrollArea->setWidget(m_rightView);
    rightHolderLayout->addWidget(m_rightViewScrollArea);

    //TODO:
    //    Add record area code here
    m_recordBarHolder = new QWidget(m_rightViewHolder);
    m_recordBarHolder->setFixedHeight(86);
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
    //初始化详情页面为当前笔记
    m_stackedRightMainWidget->setCurrentWidget(m_rightViewHolder);

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
    m_a2tManager = new VNoteA2TManager(this);

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
            //搜索内容不为空，切换为单选详情页面
            changeRightView(false);
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
    //记事本切换后刷新详情页
    changeRightView(false);
    VNoteFolder *data = static_cast<VNoteFolder *>(StandardItemCommon::getStandardItemData(current));
    if (!loadNotes(data)) {
        m_stackedRightMainWidget->setCurrentWidget(m_rightViewHolder);
        m_rightView->initData(nullptr, m_searchKey, false);
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
    QList<QWidget *> Children {m_middleViewHolder, m_stackedRightMainWidget};

    for (auto it : Children) {
        QSplitterHandle *handle = m_mainWndSpliter->handle(m_mainWndSpliter->indexOf(it));
        if (handle) {
            handle->setFixedWidth(2);
            handle->setDisabled(true);
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
    if (voiceSize >= 1000) {
        m_rightView->insertVoiceItem(voicePath, voiceSize);

        //Recording normal,remove the data safer.
        VNoteItem *currentNote = m_middleView->getCurrVNotedata();

        if (nullptr != currentNote && !voicePath.isEmpty()) {
            VDataSafer safer;
            safer.setSaferType(VDataSafer::Unsafe);

            safer.folder_id = currentNote->folderId;
            safer.note_id = currentNote->noteId;
            safer.path = voicePath;
            if (!stateOperation->isAppQuit()) {
                VNoteDataSafefyManager::instance()->doSafer(safer);
            } else { //需要关闭应用时,同步更新数据库
                VNoteSaferOper saferOper;
                saferOper.rmSafer(safer);
            }
        } else {
            qCritical() << "UnSafe get currentNote data is null! Dangerous!!!";
        }
    }
    setSpecialStatus(RecordEnd);

    //Release shutdonw locker
    releaseHaltLock();

    if (stateOperation->isAppQuit()) {
        release();
        exit(0);
    }
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

    VoiceNoteItem *asrVoiceItem = nullptr;

    if (first) {
        DetailItemWidget *widget = m_rightView->getOnlyOneSelectVoice();
        asrVoiceItem = static_cast<VoiceNoteItem *>(widget);
        m_rightView->setCurVoiceAsr(asrVoiceItem);
    } else {
        asrVoiceItem = m_rightView->getCurVoiceAsr();
        if (asrVoiceItem) {
            if (m_rightView->getOnlyOneSelectVoice() != asrVoiceItem) {
                m_rightView->setCurVoiceAsr(nullptr);
                return;
            }
        }
    }

    if (asrVoiceItem && asrVoiceItem->getNoteBlock()->blockText.isEmpty()) {
        VNVoiceBlock *data = asrVoiceItem->getNoteBlock()->ptrVoice;

        if (nullptr != data) {
            //Check whether the audio lenght out of 20 minutes
            if (data->voiceSize > MAX_A2T_AUDIO_LEN_MS) {
                VNoteMessageDialog audioOutLimit(
                    VNoteMessageDialog::AsrTimeLimit, this);

                audioOutLimit.exec();
            } else {
                setSpecialStatus(VoiceToTextStart);
                asrVoiceItem->showAsrStartWindow();
                QTimer::singleShot(0, this, [this, data]() {
                    m_a2tManager->startAsr(data->voicePath, data->voiceSize);
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
    VoiceNoteItem *asrVoiceItem = m_rightView->getCurVoiceAsr();
    if (asrVoiceItem) {
        asrVoiceItem->showAsrEndWindow("");
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
    VoiceNoteItem *asrVoiceItem = m_rightView->getCurVoiceAsr();
    if (asrVoiceItem) {
        m_rightView->clearAllSelection();
        asrVoiceItem->getNoteBlock()->blockText = text;
        asrVoiceItem->showAsrEndWindow(text);
        m_rightView->updateData();
    }
    setSpecialStatus(VoiceToTextEnd);
    m_rightView->setCurVoiceAsr(nullptr);
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
         it != shortcutNotebookKeymap.end(); ++it) {
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
        {DApplication::translate("Shortcuts", "Save as MP3"), "Ctrl+P"},
        {DApplication::translate("Shortcuts", "Save as TXT"), "Ctrl+S"},
        {DApplication::translate("Shortcuts", "Save recordings"), "Ctrl+Y"},
    };

    if (stateOperation->isAiSrvExist()) {
        shortcutNoteKeymap.insert(DApplication::translate("Shortcuts", "Voice to Text"), "Ctrl+W");
    }

    QJsonObject noteJsonGroup;
    noteJsonGroup.insert("groupName", DApplication::translate("ShortcutsGroups", "Notes"));
    QJsonArray noteJsonItems;

    for (QMap<QString, QString>::iterator it = shortcutNoteKeymap.begin();
         it != shortcutNoteKeymap.end(); ++it) {
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
         it != shortcutEditKeymap.end(); ++it) {
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
         it != shortcutSettingKeymap.end(); ++it) {
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

    QProcess *shortcutViewProcess = new QProcess(this);
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

    Qt::WindowStates states = windowState();
    m_needShowMax = states.testFlag(Qt::WindowMaximized);

    //最大化状态同时拥有Qt::WindowActive状态会导致最大化图标错误,去除该状态
    if (m_needShowMax && states.testFlag(Qt::WindowActive)) {
        states.setFlag(Qt::WindowActive, false);
        setWindowState(states);
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

    QScrollBar *bar = m_rightViewScrollArea->verticalScrollBar();
    bar->setValue(bar->minimum());
    m_rightView->initData(data, m_searchKey, m_rightViewHasFouse);
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
        if (VNoteBaseDialog::Accepted == confirmDialog.exec()) {
            delNotepad();
        }
    } break;
    case ActionManager::NotebookAddNew:
        addNote();
        break;
    case ActionManager::NoteDelete: {
        VNoteMessageDialog confirmDialog(VNoteMessageDialog::DeleteNote, this, m_middleView->getSelectedCount());
        if (VNoteBaseDialog::Accepted == confirmDialog.exec()) {
            delNote();
        }
    } break;
    case ActionManager::NoteAddNew:
        addNote();
        break;
    case ActionManager::NoteRename:
        editNote();
        break;
    case ActionManager::DetailDelete: {
        int ret = m_rightView->showWarningDialog();
        if (ret == 1) {
            VNoteMessageDialog confirmDialog(VNoteMessageDialog::DeleteNote, this);
            if (VNoteBaseDialog::Accepted == confirmDialog.exec()) {
                m_rightView->delSelectText();
            }
        } else if (ret == 0) {
            m_rightView->delSelectText();
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
    case ActionManager::DetailSelectAll:
        m_rightView->selectAllItem();
        break;
    case ActionManager::DetailCopy:
        m_rightView->copySelectText();
        break;
    case ActionManager::DetailPaste:
        m_rightView->pasteText();
        break;
    case ActionManager::DetailCut:
        m_rightView->cutSelectText();
        break;
    case ActionManager::DetailVoiceSave:
        m_rightView->saveMp3();
        break;
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
    case ActionManager::NoteTop:
        m_middleView->noteStickOnTop();
        break;
    case ActionManager::NoteMove: {
        //删除前记录选中位置
        m_middleView->setNextSelection();

        QModelIndexList notesdataList = m_middleView->getAllSelectNote();
        if (notesdataList.size()) {
            QModelIndex selectFolder = m_leftView->selectMoveFolder(notesdataList);
            m_leftView->setNumberOfNotes(m_middleView->count());
            if (selectFolder.isValid() && m_leftView->doNoteMove(notesdataList, selectFolder)) {
                m_middleView->deleteModelIndexs(notesdataList);
                //删除后设置选中
                m_middleView->selectAfterRemoved();
            }
        }
    } break;
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
    QAction *topAction = ActionManager::Instance()->getActionById(ActionManager::NoteTop);
    if (menu == ActionManager::Instance()->noteContextMenu()) {
        bool notMultipleSelected = !m_middleView->isMultipleSelected();
        ActionManager::Instance()->visibleAction(ActionManager::NoteTop, notMultipleSelected);
        ActionManager::Instance()->visibleAction(ActionManager::NoteRename, notMultipleSelected);

        ActionManager::Instance()->resetCtxMenu(ActionManager::MenuType::NoteCtxMenu);
        if (stateOperation->isPlaying()
            || stateOperation->isRecording()
            || stateOperation->isVoice2Text()
            || stateOperation->isSearching()) {
            ActionManager::Instance()->enableAction(ActionManager::NoteAddNew, false);
            if (!stateOperation->isSearching()) {
                ActionManager::Instance()->enableAction(ActionManager::NoteDelete, false);
            } else {
                topAction->setEnabled(false);
            }
            if (stateOperation->isRecording()) {
                ActionManager::Instance()->enableAction(ActionManager::NoteSaveVoice, false);
            }
            ActionManager::Instance()->enableAction(ActionManager::NoteMove, false);
        }

        //Disable SaveText if note have no text
        //Disable SaveVoice if note have no voice.
        //只有当前一个记事本，右键移动置灰
        if (1 == m_leftView->folderCount()) {
            ActionManager::Instance()->enableAction(ActionManager::NoteMove, false);
        }
        if (m_middleView->isMultipleSelected()) {
            if (!m_middleView->haveText()) {
                ActionManager::Instance()->enableAction(ActionManager::NoteSaveText, false);
            }
            if (!m_middleView->haveVoice()) {
                ActionManager::Instance()->enableAction(ActionManager::NoteSaveVoice, false);
            }
        } else {
            VNoteItem *currNoteData = m_middleView->getCurrVNotedata();

            if (nullptr != currNoteData) {
                if (!currNoteData->haveText()) {
                    ActionManager::Instance()->enableAction(ActionManager::NoteSaveText, false);
                }

                if (!currNoteData->haveVoice()) {
                    ActionManager::Instance()->enableAction(ActionManager::NoteSaveVoice, false);
                }
                if (currNoteData->isTop) {
                    topAction->setText(DApplication::translate("NotesContextMenu", "Unstick"));
                } else {
                    topAction->setText(DApplication::translate("NotesContextMenu", "Sticky on Top"));
                }
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
    QVariant value = setting::instance()->getOption(VNOTE_FOLDER_SORT);
    QStringList tmpsortFolders = value.toString().split(",");

    int folderCount = 0;

    if (folders) {
        folders->lock.lockForRead();

        folderCount = folders->folders.size();

        for (auto it : folders->folders) {
            int tmpIndexCount = tmpsortFolders.indexOf(QString::number(it->id));
            if (-1 != tmpIndexCount) {
                it->sortNumber = folderCount - tmpIndexCount;
            }
            m_leftView->appendFolder(it);
        }

        folders->lock.unlock();

        m_leftView->sort();
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

        bool sortEnable = false;
        VNoteFolder *tmpFolder = m_leftView->getFirstFolder();
        if (tmpFolder && -1 != tmpFolder->sortNumber) {
            newFolder->sortNumber = tmpFolder->sortNumber + 1;
            sortEnable = true;
        }

        m_leftView->addFolder(newFolder);
        m_leftView->sort();

        if (sortEnable) {
            QString folderSortData = m_leftView->getFolderSort();
            setting::instance()->setOption(VNOTE_FOLDER_SORT, folderSortData);
        }

        addNote();
    }
}

/**
 * @brief VNoteMainWindow::delNotepad
 */
void VNoteMainWindow::delNotepad()
{
    VNoteFolder *data = m_leftView->removeFolder();

    // 判断当前删除的记事本的排序编号，如果不是-1，则将当前所有记事本的排序编号写入配置文件中
    if (-1 != data->sortNumber) {
        QString folderSortData = m_leftView->getFolderSort();
        setting::instance()->setOption(VNOTE_FOLDER_SORT, folderSortData);
    }

    m_rightView->removeCacheWidget(data);

    VNoteFolderOper folderOper(data);
    folderOper.deleteVNoteFolder(data);

    if (0 == m_leftView->folderCount()) {
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
    m_middleView->clearSelection();

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
        m_rightView->saveNote();
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
 * 删除记事项
 */
void VNoteMainWindow::delNote()
{
    m_stackedRightMainWidget->setCurrentWidget(m_rightViewHolder);
    //记录移除前位置
    m_middleView->setNextSelection();

    QList<VNoteItem *> noteDataList = m_middleView->deleteCurrentRow();

    if (noteDataList.size()) {
        for (auto noteData : noteDataList) {
            m_rightView->removeCacheWidget(noteData);
            VNoteItemOper noteOper(noteData);
            noteOper.deleteNote();
        }
        //Refresh the middle view
        if (m_middleView->rowCount() <= 0 && stateOperation->isSearching()) {
            m_middleView->setVisibleEmptySearch(true);
        }

        //Refresh the notes count of folder
        m_leftView->update(m_leftView->currentIndex());
    }
    //设置移除后选中
    m_middleView->selectAfterRemoved();
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
            m_stackedRightMainWidget->setCurrentWidget(m_rightViewHolder);
            m_rightView->initData(nullptr, m_searchKey);
            m_recordBar->setVisible(false);
        } else {
            m_middleView->sortView(false);
            m_middleView->setVisibleEmptySearch(false);
            m_middleView->setCurrentIndex(0);
        }
        //刷新详情页-切换至当前笔记
        m_stackedRightMainWidget->setCurrentWidget(m_rightViewHolder);
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
    VoiceNoteItem *voiceItem = m_rightView->getCurVoicePlay();
    if (voiceItem && voiceItem->getNoteBlock() == voiceData) {
        voiceItem->showPauseBtn();
    }
}

/**
 * @brief VNoteMainWindow::onPlayPlugVoicePause
 * @param voiceData
 */
void VNoteMainWindow::onPlayPlugVoicePause(VNVoiceBlock *voiceData)
{
    VoiceNoteItem *voiceItem = m_rightView->getCurVoicePlay();
    if (voiceItem && voiceItem->getNoteBlock() == voiceData) {
        voiceItem->showPlayBtn();
    }
}

/**
 * @brief VNoteMainWindow::onPlayPlugVoiceStop
 * @param voiceData
 */
void VNoteMainWindow::onPlayPlugVoiceStop(VNVoiceBlock *voiceData)
{
    VoiceNoteItem *voiceItem = m_rightView->getCurVoicePlay();
    if (voiceItem && voiceItem->getNoteBlock() == voiceData) {
        voiceItem->showPlayBtn();
    }
    setSpecialStatus(PlayVoiceEnd);
    m_rightView->setCurVoicePlay(nullptr);
    m_rightView->setFocus();
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
        m_rightView->setEnablePlayBtn(false);
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
        m_rightView->setEnablePlayBtn(true);
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
        m_rightView->closeMenu();
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
    bool searchEnable = type == WndNoteShow ? true : false;
    if (!searchEnable) {
        titlebar()->setFocus(Qt::TabFocusReason);
    }
    m_noteSearchEdit->setEnabled(searchEnable);
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
    m_rightView->saveNote();

    QScopedPointer<VNoteA2TManager> releaseA2TManger(m_a2tManager);
    if (stateOperation->isVoice2Text()) {
        releaseA2TManger->stopAsr();
    }
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
            url = "https://www.deepin.org/zh/agreement/privacy/";
        } else {
            url = "https://www.uniontech.com/agreement/privacy-cn";
        }
    } else {
        if (isCommunityEdition) {
            url = "https://www.deepin.org/en/agreement/privacy/";
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

/**
 * @brief MiddleView::onDropNote
 * @param dropCancel
 * 笔记拖拽结束
 */
void VNoteMainWindow::onDropNote(bool dropCancel)
{
    //取消拖拽，return
    if (dropCancel) {
        m_middleView->setDragSuccess(false);
        return;
    }
    QModelIndexList indexList = m_middleView->getAllSelectNote();
    QPoint point = m_leftView->mapFromGlobal(QCursor::pos());
    QModelIndex selectIndex = m_leftView->indexAt(point);
    m_leftView->setNumberOfNotes(m_middleView->count());
    bool ret = m_leftView->doNoteMove(indexList, selectIndex);
    if (ret) {
        m_middleView->deleteModelIndexs(indexList);
    }
    //拖拽完成
    m_middleView->setDragSuccess(true);
}

/**
 * @brief MiddleView::handleMultipleOption
 * @param id
 * 响应多选详情页操作
 */
void VNoteMainWindow::handleMultipleOption(int id)
{
    switch (id) {
    case ButtonValue::Move: {
        //多选页面-移动
        m_middleView->setNextSelection();
        QModelIndexList notesdata = m_middleView->getAllSelectNote();
        if (notesdata.size()) {
            QModelIndex selectFolder = m_leftView->selectMoveFolder(notesdata);
            m_leftView->setNumberOfNotes(m_middleView->count());
            if (selectFolder.isValid() && m_leftView->doNoteMove(notesdata, selectFolder)) {
                m_middleView->deleteModelIndexs(notesdata);
                //移除后选中
                m_middleView->selectAfterRemoved();
            }
        }
    } break;
    case ButtonValue::SaveAsTxT:
        //多选页面-保存TxT
        m_middleView->saveAsText();
        break;
    case ButtonValue::SaveAsVoice:
        //多选页面-保存语音
        m_middleView->saveRecords();
        break;
    case ButtonValue::Delete:
        //多选页面-删除
        VNoteMessageDialog confirmDialog(VNoteMessageDialog::DeleteNote, this, m_middleView->getSelectedCount());
        if (VNoteBaseDialog::Accepted == confirmDialog.exec()) {
            delNote();
        }
        break;
    }
}

/**
 * @brief VNoteMainWindow::setTitleBarTabFocus
 * @param event
 */
void VNoteMainWindow::setTitleBarTabFocus(QKeyEvent *event)
{
    titlebar()->setFocus(Qt::TabFocusReason);
    if (event != nullptr) {
        DWidget::keyPressEvent(event);
    }
}

/**
 * @brief VNoteMainWindow::eventFilter
 * @param o
 * @param e
 * @return
 */
bool VNoteMainWindow::eventFilter(QObject *o, QEvent *e)
{
    if (e->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = dynamic_cast<QKeyEvent *>(e);
        if (keyEvent->key() == Qt::Key_Tab) {
            return setTabFocus(o, keyEvent);
        } else if (o == m_addNotepadBtn && (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)) {
            addNotepad();
            return true;
        }
    }
    if (o == m_noteSearchEdit->lineEdit() && e->type() == QEvent::FocusIn) {
        m_showSearchEditMenu = false;
    }
    return false;
}

/**
 * @brief VNoteMainWindow::setTabFocus
 * @param obj
 * @param event
 * @return
 */
bool VNoteMainWindow::setTabFocus(QObject *obj, QKeyEvent *event)
{
    if (obj == m_rightView) {
        return true;
    } else if (obj == m_noteSearchEdit->lineEdit()) {
        return false;
    } else if (obj == m_addNoteBtn) {
        return setAddnoteButtonNext(event);
    } else if (obj == m_middleView) {
        return setMiddleviewNext(event);
    } else if (obj == m_addNotepadBtn) {
        return setAddnotepadButtonNext(event);
    } else {
        return setTitleCloseButtonNext(event);
    }
}

/**
 * @brief VNoteMainWindow::setAddnotepadButtonNext
 * @param event
 * @return
 */
bool VNoteMainWindow::setAddnotepadButtonNext(QKeyEvent *event)
{
    if (m_middleView->rowCount()) {
        return false;
    }
    if (m_addNoteBtn->isEnabled() && m_addNoteBtn->isVisible()) {
        m_addNoteBtn->setFocus(Qt::TabFocusReason);
        return true;
    }

    return setMiddleviewNext(event);
}

/**
 * @brief VNoteMainWindow::setAddnoteButtonNext
 * @param event
 * @return
 */
bool VNoteMainWindow::setAddnoteButtonNext(QKeyEvent *event)
{
    if (m_middleView->rowCount() == 0) {
        return setTitlebarNext(event);
    }
    if (m_rightView->getIsNormalView()) {
        m_recordBar->setFocus(Qt::TabFocusReason);
    } else {
        m_multipleSelectWidget->setFocus(Qt::TabFocusReason);
    }
    DWidget::keyPressEvent(event);
    return true;
}

/**
 * @brief VNoteMainWindow::setTitleCloseButtonNext
 * @param event
 * @return
 */
bool VNoteMainWindow::setTitleCloseButtonNext(QKeyEvent *event)
{
    if (stateOperation->isSearching()) {
        if (m_middleView->searchEmpty()) {
            m_noteSearchEdit->lineEdit()->setFocus(Qt::TabFocusReason);
            return true;
        }
        m_middleView->setFocus(Qt::TabFocusReason);
        return true;
    }
    m_stackedWidget->currentWidget()->setFocus();
    DWidget::keyPressEvent(event);
    return true;
}

/**
 * @brief VNoteMainWindow::setTitlebarNext
 * @param event
 * @return
 */
bool VNoteMainWindow::setTitlebarNext(QKeyEvent *event)
{
    Q_UNUSED(event)
    if (m_noteSearchEdit->isEnabled()) {
        m_noteSearchEdit->lineEdit()->setFocus(Qt::TabFocusReason);
    } else {
        return false;
    }
    return true;
}

/**
 * @brief VNoteMainWindow::setMiddleviewNext
 * @param event
 * @return
 */
bool VNoteMainWindow::setMiddleviewNext(QKeyEvent *event)
{
    if (stateOperation->isSearching()) {
        m_rightView->setFocus(Qt::TabFocusReason);
        DWidget::keyPressEvent(event);
        return true;
    }
    if (stateOperation->isRecording() || stateOperation->isPlaying() || stateOperation->isVoice2Text()) {
        m_recordBar->setFocus(Qt::TabFocusReason);
        DWidget::keyPressEvent(event);
        return true;
    }
    return false;
}

/**
 * @brief VNoteMainWindow::needShowMax
 * @return
 */
bool VNoteMainWindow::needShowMax()
{
    return m_needShowMax;
}

void VNoteMainWindow::onDeleteShortcut()
{
    if (canDoShortcutAction()) {
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
        } else if (m_rightView->hasFocus()) {
            deleteAct = ActionManager::Instance()->getActionById(
                ActionManager::DetailDelete);
        } else {
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
}
