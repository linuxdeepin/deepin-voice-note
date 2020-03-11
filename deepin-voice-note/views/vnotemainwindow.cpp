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
#include "common/vnoteaudiodevicewatcher.h"
#include "common/vnotea2tmanager.h"
#include "common/vnoteitem.h"
#include "common/vnoteforlder.h"
#include "common/actionmanager.h"

#include "common/utils.h"
#include "common/actionmanager.h"

#include "db/vnotefolderoper.h"
#include "db/vnoteitemoper.h"

#include "dialog/vnotemessagedialog.h"
#include "views/vnoterecordbar.h"
#include "widgets/vnoteiconbutton.h"

#include "globaldef.h"

#include <QScrollBar>

#include <DApplication>
#include <DApplicationHelper>
#include <DFontSizeManager>
#include <DLog>
#include <DStatusBar>
#include <DTitlebar>

VNoteMainWindow::VNoteMainWindow(QWidget *parent)
    : DMainWindow(parent)
{
    initUI();
    initConnections();
    initShortcuts();
    // Request DataManager load  note folders
    initData();

    //Start audio device watch thread
    //& must be called after initUI
    initAudioWatcher();
}

VNoteMainWindow::~VNoteMainWindow()
{
    //Save main window size
    if (!isMaximized()) {
        m_qspSetting->setValue(VNOTE_MAINWND_SZ_KEY, saveGeometry());
    }
    //TODO:
    //    Nofiy audio watcher to exit & need
    //wait at least AUDIO_DEV_CHECK_TIME ms
    QScopedPointer<VNoteAudioDeviceWatcher> autoRelease(
        m_audioDeviceWatcher
    );
    autoRelease->exitWatcher();
    autoRelease->wait(AUDIO_DEV_CHECK_TIME);

    QScopedPointer<VNoteA2TManager> releaseA2TManger(m_a2tManager);
    if (m_isAsrVoiceing) {
        releaseA2TManger->stopAsr();
    }
}

QSharedPointer<QSettings> VNoteMainWindow::appSetting() const
{
    return m_qspSetting;
}

void VNoteMainWindow::initUI()
{
    initTitleBar();
    initMainView();
}

void VNoteMainWindow::initData()
{
    //Init app data here
    initAppSetting();

    VNoteDataManager::instance()->reqNoteDefIcons();
    VNoteDataManager::instance()->reqNoteFolders();
    VNoteDataManager::instance()->reqNoteItems();
}

void VNoteMainWindow::initAppSetting()
{
    QString vnoteConfigBasePath =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    QFileInfo configDir(vnoteConfigBasePath);

    //TODO:
    //    Remove the old version database
    //make a app owne directory
    if (!configDir.isDir() && configDir.exists()) {
        QFile oldDbFile(vnoteConfigBasePath);
        if (!oldDbFile.remove()) {
            qInfo() << oldDbFile.fileName() << ":" << oldDbFile.errorString();
        }
    }

    configDir.setFile(vnoteConfigBasePath + QDir::separator() + QString("config/"));

    if (!configDir.exists()) {
        QDir().mkpath(configDir.filePath());
        qInfo() << "create config dir:" << configDir.filePath();
    }

    m_qspSetting.reset(new QSettings(configDir.filePath() + QString("config.conf")
                                     , QSettings::IniFormat));

}

void VNoteMainWindow::initConnections()
{
    connect(VNoteDataManager::instance(), &VNoteDataManager::onAllDatasReady,
            this, &VNoteMainWindow::onVNoteFoldersLoaded);

    connect(m_noteSearchEdit, &DSearchEdit::textChanged,
            this, &VNoteMainWindow::onVNoteSearch);

    connect(m_leftView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &VNoteMainWindow::onVNoteFolderChange);

    connect(m_middleView, SIGNAL(currentChanged(const QModelIndex &)),
            this, SLOT(onVNoteChange(const QModelIndex &)));

    connect(m_rightView, &RightView::contentChanged,
                m_middleView, &MiddleView::onNoteChanged);

    connect(m_rightView, &RightView::sigVoicePlay,
            this, &VNoteMainWindow::onRightViewVoicePlay);
    connect(m_rightView, &RightView::sigVoicePause,
            this, &VNoteMainWindow::onRightViewVoicePause);
    connect(m_rightView, &RightView::sigCursorChange,
            this, &VNoteMainWindow::onCursorChange);

    connect(m_addNotepadBtn, &DPushButton::clicked,
            this, &VNoteMainWindow::addNotepad);

    connect(m_addNoteBtn, &VNoteIconButton::clicked,
            this, &VNoteMainWindow::addNote);

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

    connect(m_asrAgainBtn, &DPushButton::clicked,
            this, &VNoteMainWindow::onA2TStart);

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
    connect(ActionManager::Instance()->voiceContextMenu(), &DMenu::aboutToShow,
            this, &VNoteMainWindow::onMenuAbout2Show);
    connect(ActionManager::Instance()->voiceContextMenu(), &DMenu::triggered,
            this, &VNoteMainWindow::onMenuAction);

}

void VNoteMainWindow::initShortcuts()
{
    m_stPreviewShortcuts.reset(new QShortcut(this));
    m_stPreviewShortcuts->setKey(QString("Ctrl+Shift+/"));
    m_stPreviewShortcuts->setContext(Qt::ApplicationShortcut);
    m_stPreviewShortcuts->setAutoRepeat(false);

    connect(m_stPreviewShortcuts.get(), &QShortcut::activated,
            this, &VNoteMainWindow::onPreviewShortcut);
}

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
    // Search note
    m_noteSearchEdit = new DSearchEdit(this);
    DFontSizeManager::instance()->bind(m_noteSearchEdit, DFontSizeManager::T6);
    m_noteSearchEdit->setFixedSize(QSize(VNOTE_SEARCHBAR_W, VNOTE_SEARCHBAR_H));
    m_noteSearchEdit->setPlaceHolder(DApplication::translate("TitleBar", "Search"));
    titlebar()->addWidget(m_noteSearchEdit);
}

void VNoteMainWindow::initMainView()
{
    initSpliterView();
    initSplashView();
    initEmptyFoldersView();
    initAsrErrMessage();
    m_centerWidget = new DStackedWidget(this);
    m_centerWidget->setContentsMargins(0, 0, 0, 0);
    m_centerWidget->insertWidget(WndSplashAnim, m_splashView);
    m_centerWidget->insertWidget(WndHomePage, m_wndHomePage);
    m_centerWidget->insertWidget(WndNoteShow, m_mainWndSpliter);
    m_centerWidget->setCurrentIndex(WndSplashAnim);
    setCentralWidget(m_centerWidget);
    setTitlebarShadowEnabled(true);
}

void VNoteMainWindow::initLeftView()
{
    m_leftViewHolder = new QWidget(m_mainWndSpliter);
    m_leftViewHolder->setObjectName("leftMainLayoutHolder");
    m_leftViewHolder->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_leftViewHolder->setFixedWidth(VNOTE_LEFTVIEW_W);
    m_leftViewHolder->setBackgroundRole(DPalette::Base);
    m_leftViewHolder->setAutoFillBackground(true);

    QVBoxLayout *leftHolderLayout = new  QVBoxLayout();
    leftHolderLayout->setSpacing(0);
    leftHolderLayout->setContentsMargins(0, 5, 0, 0);
    m_leftView = new LeftView(m_leftViewHolder);

    m_leftView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_leftView->setContentsMargins(0, 0, 0, 0);
    m_leftView->setHeaderHidden(true);
    m_leftView->setFrameShape(QFrame::NoFrame);
    m_leftView->setItemsExpandable(false);
    m_leftView->setIndentation(0);
    QStandardItem *notepadRoot = m_leftView->getNotepadRoot();
    m_leftView->expand(notepadRoot->index());
    leftHolderLayout->addWidget(m_leftView);

    m_addNotepadBtn = new DPushButton(DApplication::translate("VNoteMainWindow", "Create notebook"),
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

void VNoteMainWindow::initMiddleView()
{
    m_middleViewHolder = new QWidget(m_mainWndSpliter);
    m_middleViewHolder->setObjectName("middleMainLayoutHolder");
    m_middleViewHolder->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_middleViewHolder->setFixedWidth(VNOTE_MIDDLEVIEW_W);
    m_middleViewHolder->setBackgroundRole(DPalette::Base);
    m_middleViewHolder->setAutoFillBackground(true);

    QVBoxLayout *middleHolderLayout = new  QVBoxLayout();
    middleHolderLayout->setSpacing(0);
    middleHolderLayout->setContentsMargins(0, 5, 0, 5);

    m_middleView = new MiddleView(m_middleViewHolder);
    m_addNoteBtn = new VNoteIconButton(
        m_middleViewHolder,
        "add_note_normal.svg",
        "add_note_hover.svg",
        "add_note_press.svg"
    );
    m_addNoteBtn->SetDisableIcon("add_note_disabled.svg");
    m_addNoteBtn->setFlat(true);
    m_addNoteBtn->setIconSize(QSize(68, 68));
    m_addNoteBtn->raise();

    DAnchorsBase buttonAnchor(m_addNoteBtn);
    buttonAnchor.setAnchor(Qt::AnchorLeft, m_middleView, Qt::AnchorLeft);
    buttonAnchor.setAnchor(Qt::AnchorBottom, m_middleView, Qt::AnchorBottom);
    buttonAnchor.setBottomMargin(0);
    buttonAnchor.setLeftMargin(97);

    // ToDo:
    //    Add Left view widget here
    middleHolderLayout->addWidget(m_middleView);

    m_middleViewHolder->setLayout(middleHolderLayout);


#ifdef VNOTE_LAYOUT_DEBUG
    m_middleView->setStyleSheet("background: green");
#endif
}

void VNoteMainWindow::initRightView()
{
    m_rightViewHolder = new QWidget(m_mainWndSpliter);
    m_rightViewHolder->setObjectName("rightMainLayoutHolder");
    m_rightViewHolder->setSizePolicy(QSizePolicy::Expanding
                                     , QSizePolicy::Expanding);
    m_rightViewHolder->setBackgroundRole(DPalette::Base);
    m_rightViewHolder->setAutoFillBackground(true);

    QVBoxLayout *rightHolderLayout = new QVBoxLayout;
    rightHolderLayout->setSpacing(0);
    rightHolderLayout->setContentsMargins(0, 20, 0, 0);

    m_rightViewScrollArea = new DScrollArea(m_rightViewHolder);
    m_rightView = new RightView(m_rightViewScrollArea);
    m_rightViewScrollArea->setLineWidth(0);
    m_rightViewScrollArea->setWidgetResizable(true);
    m_rightViewScrollArea->setWidget(m_rightView);
    rightHolderLayout->addWidget(m_rightViewScrollArea);

    //TODO:
    //    Add record area code here
    m_recordBar = new VNoteRecordBar(m_rightViewHolder);
    m_recordBar->setBackgroundRole(DPalette::Base);
    m_recordBar->setAutoFillBackground(true);
    m_recordBar->setFixedHeight(78);
    m_recordBar->setSizePolicy(QSizePolicy::Expanding
                               , QSizePolicy::Fixed);
    rightHolderLayout->addWidget(m_recordBar, Qt::AlignBottom);
    m_rightViewHolder->setLayout(rightHolderLayout);

#ifdef VNOTE_LAYOUT_DEBUG
    m_rightViewHolder->setStyleSheet("background: red");
    m_rightNoteArea->setStyleSheet("background: blue");
    m_recordBar->setStyleSheet("background: green");
#endif
}

void VNoteMainWindow::initAudioWatcher()
{
    QTimer::singleShot(50, this, [this]() {
        //Audio input device watchdog
        m_audioDeviceWatcher = new VNoteAudioDeviceWatcher();

        connect(m_audioDeviceWatcher, &VNoteAudioDeviceWatcher::microphoneAvailableState,
                m_recordBar, &VNoteRecordBar::OnMicrophoneAvailableChanged);

        m_audioDeviceWatcher->start();

        //audio to text manager
        m_a2tManager = new VNoteA2TManager();

        //connect(m_rightView, &RightView::asrStart, this, &VNoteMainWindow::onA2TStart);
        connect(m_a2tManager, &VNoteA2TManager::asrError, this, &VNoteMainWindow::onA2TError);
        connect(m_a2tManager, &VNoteA2TManager::asrSuccess, this, &VNoteMainWindow::onA2TSuccess);
    });
}

void VNoteMainWindow::onVNoteFoldersLoaded()
{
    //If have folders show note view,else show
    //default home page
    if (loadNotepads() > 0) {
        m_centerWidget->setCurrentIndex(WndNoteShow);
    } else {
        m_centerWidget->setCurrentIndex(WndHomePage);
    }
}

void VNoteMainWindow::onVNoteSearch()
{
    if (m_centerWidget->currentIndex() == WndNoteShow) {
        QString strKey = m_noteSearchEdit->text();
        if (!strKey.isEmpty()) {
            m_searchKey.setPattern(strKey);
            m_searchKey.setCaseSensitivity(Qt::CaseInsensitive);
            loadSearchNotes(m_searchKey);
            setSpecialStatus(SearchStart);
        } else {
            setSpecialStatus(SearchEnd);
        }
    }
}

void VNoteMainWindow::onVNoteFolderChange(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous);
    m_asrErrMeassage->setVisible(false);
    VNoteFolder *data = static_cast<VNoteFolder *>(StandardItemCommon::getStandardItemData(current));
    loadNotes(data);
}

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

void VNoteMainWindow::initSplashView()
{
    m_splashView = new SplashView(this);
}

void VNoteMainWindow::initEmptyFoldersView()
{
    m_wndHomePage = new HomePage(this);
}

void VNoteMainWindow::onStartRecord()
{
    setSpecialStatus(RecordStart);
}

void VNoteMainWindow::onFinshRecord(const QString &voicePath, qint64 voiceSize)
{
    if (voiceSize >= 1000) {
        m_rightView->insertVoiceItem(voicePath, voiceSize);
    } else {

    }
    setSpecialStatus(RecordEnd);
    if (m_isExit) {
        qApp->quit();
    }
}

void VNoteMainWindow::onChangeTheme()
{
    ;
}

void VNoteMainWindow::onA2TStart()
{
    m_asrErrMeassage->setVisible(false);
    m_currentAsrVoice = m_rightView->getMenuVoiceItem();

    if (m_currentAsrVoice) {
        VNVoiceBlock *data = m_currentAsrVoice->getNoteBlock();

        if (nullptr != data) {
            //Check whether the audio lenght out of 20 minutes
            if (data->voiceSize > MAX_A2T_AUDIO_LEN_MS) {
                VNoteMessageDialog audioOutLimit(
                    VNoteMessageDialog::AsrTimeLimit, this);

                audioOutLimit.exec();
            } else {
                setSpecialStatus(VoiceToTextStart);
                m_currentAsrVoice->showAsrStartWindow();
                QTimer::singleShot(0, this, [this, data]() {
                    m_a2tManager->startAsr(data->voicePath, data->voiceSize);
                });
            }
        }
    }
}

void VNoteMainWindow::onA2TError(int error)
{
    if (m_currentAsrVoice) {
        m_currentAsrVoice->showAsrEndWindow("");
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

void VNoteMainWindow::onA2TSuccess(const QString &text)
{
    if (m_currentAsrVoice) {
        m_currentAsrVoice->getNoteBlock()->blockText = text;
        m_currentAsrVoice->showAsrEndWindow(text);
        m_rightView->updateData();
    }
    setSpecialStatus(VoiceToTextEnd);
}

void VNoteMainWindow::onPreviewShortcut()
{
    QRect rect = window()->geometry();
    QPoint pos(rect.x() + rect.width() / 2,
               rect.y() + rect.height() / 2);

    QJsonObject shortcutObj;
    QJsonArray jsonGroups;

    QMap<QString, QString> shortcutKeymap = {
        {DApplication::translate("Shortcuts", "Help"),                 "F1"},
//        {"Close window",         "Alt+F4"},
        {DApplication::translate("Shortcuts", "Display shortcuts"),    "Ctrl+Shift+?"},
        {DApplication::translate("Shortcuts", "New notebook"),         "Ctrl+N"},
        {DApplication::translate("Shortcuts", "Delete notebook/note"), "Delete"},
//        {"Resize window",        "Ctrl+Alt+F"},
//        {"Find",                 "Ctrl+F"},
        {DApplication::translate("Shortcuts", "Rename notebook"),      "F2"},
        {DApplication::translate("Shortcuts", "Play/Pause"),           "Space"},
        {DApplication::translate("Shortcuts", "Select all"),           "Ctrl+A"},
        {DApplication::translate("Shortcuts", "Copy"),                 "Ctrl+C"},
        {DApplication::translate("Shortcuts", "Cut"),                  "Ctrl+X"},
        {DApplication::translate("Shortcuts", "Paste"),                "Ctrl+V"},
    };

    QJsonObject fontMgrJsonGroup;
    fontMgrJsonGroup.insert("groupName", DApplication::translate("AppMain", "Voice Notes"));
    QJsonArray fontJsonItems;

    for (QMap<QString, QString>::iterator it = shortcutKeymap.begin();
            it != shortcutKeymap.end(); it++) {
        QJsonObject jsonItem;
        jsonItem.insert("name",  it.key());
        jsonItem.insert("value", it.value().replace("Meta", "Super"));
        fontJsonItems.append(jsonItem);
    }

    fontMgrJsonGroup.insert("groupItems", fontJsonItems);
    jsonGroups.append(fontMgrJsonGroup);

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

void VNoteMainWindow::resizeEvent(QResizeEvent *event)
{
    DMainWindow::resizeEvent(event);
}

void VNoteMainWindow::closeEvent(QCloseEvent *event)
{
    if (checkIfNeedExit()) {
        if (m_isRecording) {
            m_isExit = true;
            m_recordBar->cancelRecord();
            event->ignore();
        } else {
            event->accept();
        }
    } else {
        event->ignore();
    }
}

bool VNoteMainWindow::checkIfNeedExit()
{
    QScopedPointer<VNoteMessageDialog> pspMessageDialg;

    bool bNeedExit = true;

    //Is audio converting to text
    if (m_isAsrVoiceing) {
        pspMessageDialg.reset(new VNoteMessageDialog(
                                  VNoteMessageDialog::AbortRecord,
                                  this));
    } else if (m_isRecording) { //Is recording
        pspMessageDialg.reset(new VNoteMessageDialog(
                                  VNoteMessageDialog::AborteAsr,
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

void VNoteMainWindow::onVNoteChange(const QModelIndex &previous)
{
    Q_UNUSED(previous);
    m_asrErrMeassage->setVisible(false);
    QModelIndex index = m_middleView->currentIndex();
    VNoteItem *data = static_cast<VNoteItem *>(StandardItemCommon::getStandardItemData(index));
    m_rightView->initData(data);
    if (!m_searchKey.isEmpty() && m_noteSearchEdit->isEnabled()) {
        m_noteSearchEdit->lineEdit()->setFocus();
    }
}
void VNoteMainWindow::onMenuAction(QAction *action)
{
    ActionManager::ActionKind kind = ActionManager::Instance()->getActionKind(action);
    switch (kind) {
    case ActionManager::NotebookRename:
        editNotepad();
        break;
    case ActionManager::NotebookDelete: {
        VNoteMessageDialog confirmDialog(VNoteMessageDialog::DeleteFolder);
        connect(&confirmDialog, &VNoteMessageDialog::accepted, this, [this]() {
            delNotepad();
        });

        confirmDialog.exec();
    } break;
    case ActionManager::NotebookAddNew:
        addNote();
        break;
    case ActionManager::NoteDelete: {
        VNoteMessageDialog confirmDialog(VNoteMessageDialog::DeleteNote);
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
    case ActionManager::VoiceDelete: {
        VNoteMessageDialog confirmDialog(VNoteMessageDialog::DeleteNote);
        connect(&confirmDialog, &VNoteMessageDialog::accepted, this, [this]() {
            delVoice();
        });

        confirmDialog.exec();
    } break;
    case ActionManager::VoiceConversion:
        onA2TStart();
        break;
    default:
        break;
    }
}

void VNoteMainWindow::onMenuAbout2Show()
{
    //TODO:
    //    Add context menu item disable/enable code here
    //Eg:
    //ActionManager::Instance()->getActionById(ActionManager::NoteAddNew)->setDisabled(true);
    DMenu *menu = static_cast<DMenu *>(sender());
    if (menu == ActionManager::Instance()->noteContextMenu()) {
        QAction *addAction = ActionManager::Instance()->getActionById(ActionManager::NoteAddNew);
        QAction *delAction = ActionManager::Instance()->getActionById(ActionManager::NoteDelete);
        QAction *saveAction = ActionManager::Instance()->getActionById(ActionManager::NoteSaveVoice);
        if (m_isPlaying || m_isRecording || m_isAsrVoiceing) {
            addAction->setEnabled(false);
            delAction->setEnabled(false);
            if (m_isRecording) {
                saveAction->setEnabled(false);
            } else {
                saveAction->setEnabled(true);
            }
        } else {
            if (m_searchKey.isEmpty()) {
                addAction->setEnabled(true);
            } else {
                addAction->setEnabled(false);
            }
            delAction->setEnabled(true);
            saveAction->setEnabled(true);
        }
    } else if (menu == ActionManager::Instance()->notebookContextMenu()) {
        qDebug() << "notebookContextMenu";
    } else if (menu == ActionManager::Instance()->voiceContextMenu()) {
        QAction *asrAction = ActionManager::Instance()->getActionById(ActionManager::VoiceConversion);
        QAction *delAction = ActionManager::Instance()->getActionById(ActionManager::VoiceDelete);
        VoiceNoteItem *item = m_rightView->getMenuVoiceItem();
        bool textEmpyt = item->getNoteBlock()->blockText.isEmpty();
        if (!textEmpyt || m_isAsrVoiceing) {
            asrAction->setEnabled(false);
        } else {
            asrAction->setEnabled(true);
        }
        if (m_isAsrVoiceing || m_isPlaying) {
            delAction->setEnabled(false);
        } else {
            delAction->setEnabled(true);
        }
    }
}

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

        m_leftView->setDefaultNotepadItem();
    }

    return folderCount;
}

void VNoteMainWindow::addNotepad()
{
    VNoteFolder itemData;
    VNoteFolderOper  folderOper;
    itemData.defaultIcon = folderOper.getDefaultIcon();
    itemData.UI.icon = folderOper.getDefaultIcon(itemData.defaultIcon);
    itemData.name = folderOper.getDefaultFolderName();

    VNoteFolder *newFolder = folderOper.addFolder(itemData);

    if (newFolder) {
        //Switch to note view
        m_noteSearchEdit->clearEdit();
        m_centerWidget->setCurrentIndex(WndNoteShow);

        m_leftView->addFolder(newFolder);
        addNote();
    }
}

void VNoteMainWindow::delNotepad()
{
    VNoteFolder *data = m_leftView->removeFolder();
    VNoteFolderOper  folderOper(data);

    folderOper.deleteVNoteFolder(data);

    if (m_leftView->folderCount() == 0) {
        m_centerWidget->setCurrentIndex(WndHomePage);
    }
}

void VNoteMainWindow::editNotepad()
{
    m_leftView->editFolder();
}

void VNoteMainWindow::addNote()
{
    qint64 id = m_middleView->getCurrentId();
    if (id != -1) {
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

        m_middleView->addRowAtHead(newNote);
    }
}

void VNoteMainWindow::editNote()
{
    m_middleView->editNote();
}

void VNoteMainWindow::delNote()
{
    VNoteItem *noteData = m_middleView->deleteCurrentRow();

    if (noteData) {
        VNoteItemOper noteOper(noteData);
        noteOper.deleteNote();

        //Refresh the notes count of folder
        m_leftView->update(m_leftView->currentIndex());
    }
}
void VNoteMainWindow::loadNotes(VNoteFolder *folder)
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
}

void VNoteMainWindow::loadSearchNotes(const QRegExp &key)
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
        } else {
            m_middleView->setVisibleEmptySearch(false);
            m_middleView->setCurrentIndex(0);
        }
    }
}

void VNoteMainWindow::onRightViewVoicePlay(VNVoiceBlock *voiceData)
{
    setSpecialStatus(PlayVoiceStart);
    m_recordBar->playVoice(voiceData);
}

void VNoteMainWindow::onRightViewVoicePause(VNVoiceBlock *voiceData)
{
    m_recordBar->pauseVoice(voiceData);
}

void VNoteMainWindow::onPlayPlugVoicePlay(VNVoiceBlock *voiceData)
{
    VoiceNoteItem *voiceItem = m_rightView->getVoiceItem(voiceData);
    if (voiceItem) {
        voiceItem->showPauseBtn();
    }
}

void VNoteMainWindow::onPlayPlugVoicePause(VNVoiceBlock *voiceData)
{
    VoiceNoteItem *voiceItem = m_rightView->getVoiceItem(voiceData);
    if (voiceItem) {
        voiceItem->showPlayBtn();
    }
}

void VNoteMainWindow::onPlayPlugVoiceStop(VNVoiceBlock *voiceData)
{
    VoiceNoteItem *voiceItem = m_rightView->getVoiceItem(voiceData);
    if (voiceItem) {
        voiceItem->showPlayBtn();
    }
    setSpecialStatus(PlayVoiceEnd);
}

void VNoteMainWindow::delVoice()
{
    VoiceNoteItem *voiceItem = m_rightView->getMenuVoiceItem();
    if (voiceItem) {
        if (m_asrErrMeassage->isVisible() && voiceItem == m_currentAsrVoice) {
            m_asrErrMeassage->setVisible(false);
            m_currentAsrVoice = nullptr;
        }
        if (m_recordBar->stopVoice(voiceItem->getNoteBlock())) {
            setSpecialStatus(PlayVoiceEnd);
        }
        m_rightView->delWidget(voiceItem);
    }
}

void VNoteMainWindow::setSpecialStatus(SpecialStatus status)
{
    switch (status) {
    case SearchStart:
        m_leftView->clearSelection();
        m_leftView->setEnabled(false);
        m_addNotepadBtn->setVisible(false);
        m_addNoteBtn->setVisible(false);
        break;
    case SearchEnd:
        m_searchKey = QRegExp();
        m_middleView->setSearchKey(m_searchKey);
        m_leftView->setEnabled(true);
        m_addNotepadBtn->setVisible(true);
        m_addNoteBtn->setVisible(true);
        onVNoteFolderChange(m_leftView->setDefaultNotepadItem(), QModelIndex());
        break;
    case PlayVoiceStart:
        m_isPlaying = true;
        m_noteSearchEdit->setEnabled(false);
        m_leftViewHolder->setEnabled(false);
        m_middleView->setOnlyCurItemMenuEnable(true);
        m_addNoteBtn->setBtnDisabled(true);
        break;
    case PlayVoiceEnd:
        m_isPlaying = false;
        if (!m_isAsrVoiceing) {
            m_noteSearchEdit->setEnabled(true);
            m_leftViewHolder->setEnabled(true);
            m_middleView->setOnlyCurItemMenuEnable(false);
            m_addNoteBtn->setBtnDisabled(false);
        }
        break;
    case RecordStart:
        m_isRecording = true;
        m_noteSearchEdit->setEnabled(false);
        m_leftViewHolder->setEnabled(false);
        m_middleView->setOnlyCurItemMenuEnable(true);
        m_rightView->setEnablePlayBtn(false);
        m_addNoteBtn->setBtnDisabled(true);
        break;
    case RecordEnd:
        if (!m_isAsrVoiceing) {
            m_noteSearchEdit->setEnabled(true);
            m_leftViewHolder->setEnabled(true);
            m_middleView->setOnlyCurItemMenuEnable(false);
            m_addNoteBtn->setBtnDisabled(false);
        }
        m_rightView->setEnablePlayBtn(true);
        m_isRecording = false;
        break;
    case VoiceToTextStart:
        m_isAsrVoiceing = true;
        m_noteSearchEdit->setEnabled(false);
        m_leftViewHolder->setEnabled(false);
        m_middleView->setOnlyCurItemMenuEnable(true);
        m_addNoteBtn->setBtnDisabled(true);
        break;
    case VoiceToTextEnd:
        if (!m_isRecording && !m_isPlaying) {
            m_noteSearchEdit->setEnabled(true);
            m_leftViewHolder->setEnabled(true);
            m_middleView->setOnlyCurItemMenuEnable(false);
            m_addNoteBtn->setBtnDisabled(false);
        }
        m_isAsrVoiceing = false;
        break;
    default:
        break;
    }
}

void VNoteMainWindow::initAsrErrMessage()
{
    m_asrErrMeassage = new DFloatingMessage(DFloatingMessage::ResidentType,
                                            m_rightViewHolder);
    m_asrErrMeassage->setIcon(QIcon(":/images/icon/normal/warning .svg"));
    m_asrAgainBtn = new DPushButton(m_asrErrMeassage);

    m_asrAgainBtn->setText(DApplication::translate(
                               "VNoteErrorMessage",
                               "Try Again"));
    m_asrErrMeassage->setWidget(m_asrAgainBtn);
    m_asrErrMeassage->setVisible(false);
}

void VNoteMainWindow::showAsrErrMessage(const QString &strMessage)
{
    m_asrErrMeassage->setMessage(strMessage);
    m_asrErrMeassage->setVisible(true);
    m_asrErrMeassage->setFixedWidth(m_rightViewHolder->width());
    m_asrErrMeassage->adjustSize();
}

void VNoteMainWindow::onCursorChange(int height, bool mouseMove)
{
    QScrollBar *bar = m_rightViewScrollArea->verticalScrollBar();
    if (height > bar->value() + m_rightViewScrollArea->height()) {
        int value = height - m_rightViewScrollArea->height();
        if(!mouseMove){
            if(value > bar->maximum()){
                bar->setMaximum(value);
            }
        }
        bar->setValue(value);
    }
    int value = bar->value();
    if(value > height){
        bar->setValue(height );
    }
}
