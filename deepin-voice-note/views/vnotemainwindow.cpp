#include "views/vnotemainwindow.h"
#include "views/leftview.h"
#include "views/middleview.h"
#include "views/rightview.h"
#include "views/homepage.h"

#include "common/standarditemcommon.h"
#include "common/vnotedatamanager.h"
#include "common/vnoteaudiodevicewatcher.h"
#include "common/vnotea2tmanager.h"
#include "common/vnoteitem.h"
#include "common/vnoteforlder.h"

#include "common/utils.h"
#include "common/actionmanager.h"

#include "db/vnotefolderoper.h"
#include "db/vnoteitemoper.h"

#include "dialog/vnotemessagedialog.h"
#include "views/vnoterecordbar.h"
#include "widgets/vnoteiconbutton.h"

#include "globaldef.h"

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
    initAsrErrMessage();
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
    connect(VNoteDataManager::instance(), &VNoteDataManager::onNoteFoldersLoaded,
            this, &VNoteMainWindow::onVNoteFoldersLoaded);

    connect(m_noteSearchEdit, &DSearchEdit::textChanged,
            this, &VNoteMainWindow::onVNoteSearch);

    connect(m_leftView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &VNoteMainWindow::onVNoteFolderChange);
    connect(m_leftView, &LeftView::sigAction,
            this, &VNoteMainWindow::onAction);

    connect(m_middleView, &MiddleView::sigAction,
            this, &VNoteMainWindow::onAction);
    connect(m_middleView, SIGNAL(currentChanged(const QModelIndex &)),
                this, SLOT(onVNoteChange(const QModelIndex &)));


    connect(m_addNotepadBtn, &DPushButton::clicked,
            this, &VNoteMainWindow::addNotepad);

    connect(m_addNoteBtn, &VNoteIconButton::clicked,
            this, &VNoteMainWindow::addNote);

    connect(m_wndHomePage, &HomePage::sigAddFolderByInitPage,
            this, &VNoteMainWindow::addNotepad);

    connect(m_returnBtn, &DIconButton::clicked,
            this, &VNoteMainWindow::onTextEditReturn);

//    connect(m_rightView, &RightView::sigTextEditDetail,
//            this, &VNoteMainWindow::onTextEditDetail);
//    connect(m_rightView, &RightView::sigSearchNoteEmpty,
//            this, &VNoteMainWindow::onSearchNoteEmpty);
//    connect(m_rightView, &RightView::sigMenuNoteItemChange,
//            this, &VNoteMainWindow::onMenuNoteItemChange);

    connect(m_recordBar, &VNoteRecordBar::sigStartRecord, this, &VNoteMainWindow::onStartRecord);
    connect(m_recordBar, &VNoteRecordBar::sigFinshRecord, this, &VNoteMainWindow::onFinshRecord);

    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this,
            &VNoteMainWindow::onChangeTheme);

    connect(m_asrAgainBtn, &DPushButton::clicked, this, &VNoteMainWindow::onAsrAgain);

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
    m_returnBtn = new DIconButton(DStyle::SP_ArrowLeave, this);
    m_returnBtn->setFixedSize(QSize(36, 36));
    m_returnBtn->setVisible(false);
    this->titlebar()->addWidget(m_returnBtn, Qt::AlignLeft);
}

void VNoteMainWindow::initMainView()
{
    initSpliterView();
    initEmptySearchView();
    initEmptyFoldersView();
    initTextEditDetailView();
    m_centerWidget = new DStackedWidget(this);
    m_centerWidget->setContentsMargins(0, 0, 0, 0);
    m_centerWidget->insertWidget(WndHomePage, m_wndHomePage);
    m_centerWidget->insertWidget(WndNoteShow, m_mainWndSpliter);
    m_centerWidget->insertWidget(WndSearchEmpty, m_labSearchEmpty);
    m_centerWidget->insertWidget(WndTextEdit, m_textEditMainWnd);
    m_centerWidget->setCurrentIndex(WndNoteShow);
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

    m_addNotepadBtn = new DPushButton(DApplication::translate("VNoteMainWindow", "Add Notepad"),
                                      m_leftViewHolder);
    QVBoxLayout *btnLayout = new QVBoxLayout(this);
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
    rightHolderLayout->setContentsMargins(0, 10, 0, 0);

    m_rightView = new RightView(m_rightViewHolder);
    rightHolderLayout->addWidget(m_rightView);

    //TODO:
    //    Add record area code here
    m_recordBar = new VNoteRecordBar(m_rightViewHolder);
    m_recordBar->setBackgroundRole(DPalette::Base);
    m_recordBar->setAutoFillBackground(true);
    m_recordBar->setFixedHeight(78);
    m_recordBar->setSizePolicy(QSizePolicy::Expanding
                               , QSizePolicy::Fixed);
    rightHolderLayout->addWidget(m_recordBar,Qt::AlignBottom);
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
    loadNotepads();
}

void VNoteMainWindow::onVNoteSearch()
{
    QString strKey = m_noteSearchEdit->text();
    if (!strKey.isEmpty()) {
        m_searchKey.setPattern(strKey);
        m_searchKey.setCaseSensitivity(Qt::CaseInsensitive);
        loadSearchNotes(m_searchKey);
        m_leftView->clearSelection();
        m_leftView->setEnabled(false);
        m_addNotepadBtn->setVisible(false);
        m_addNoteBtn->setVisible(false);
    } else {
        m_searchKey = QRegExp();
        m_middleView->setSearchKey(m_searchKey);
        m_leftView->setEnabled(true);
        m_addNotepadBtn->setVisible(true);
        m_addNoteBtn->setVisible(true);
        QModelIndex index = m_leftView->setDefaultNotepadItem();
        onVNoteFolderChange(index, QModelIndex());
    }
}

void VNoteMainWindow::onVNoteFolderChange(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous);
    VNoteFolder *data = static_cast<VNoteFolder *>(StandardItemCommon::getStandardItemData(current));
    if (data) {
        loadNotes(data->id);
    } else {
        loadNotes(-1);
    }
}

void VNoteMainWindow::initTextEditDetailView()
{
    m_textEditMainWnd = new DTextEdit(this);
    DStyle::setFocusRectVisible(m_textEditMainWnd, false);
    DFontSizeManager::instance()->bind(m_textEditMainWnd, DFontSizeManager::T8);
    m_textEditFormat = m_textEditMainWnd->currentCharFormat();
    onChangeTheme();
}

void VNoteMainWindow::initEmptySearchView()
{
    m_labSearchEmpty = new DLabel(this);
    m_labSearchEmpty->setText(DApplication::translate("NoteSearch", "No search results"));
    DFontSizeManager::instance()->bind(m_labSearchEmpty, DFontSizeManager::T4);
    m_labSearchEmpty->setAlignment(Qt::AlignCenter);
    m_labSearchEmpty->setFocusPolicy(Qt::NoFocus);
}

void VNoteMainWindow::initSpliterView()
{
    m_mainWndSpliter = new DSplitter(Qt::Horizontal, this);
    m_mainWndSpliter->setHandleWidth(2);
    initLeftView();
    initMiddleView();
    initRightView();
}

void VNoteMainWindow::initEmptyFoldersView()
{
    m_wndHomePage = new HomePage(this);
}

void VNoteMainWindow::onTextEditDetail(VNoteItem *textNode, DTextEdit *preTextEdit, const QRegExp &searchKey)
{
    m_textNode = textNode;
    m_textEditRightView = preTextEdit;
    m_textEditMainWnd->setCurrentCharFormat(m_textEditFormat);
    m_textEditMainWnd->setPlainText(preTextEdit->toPlainText());
    bool readOnly = false;
    if (textNode->noteType == VNoteItem::VNT_Voice) { //语音项只读
        readOnly = true;
        m_noteSearchEdit->setEnabled(false);
    }
    m_textEditMainWnd->setReadOnly(readOnly);
    if (!searchKey.isEmpty()) {
        Utils::highTextEdit(m_textEditMainWnd, m_textEditFormat, searchKey, QColor(0x349ae8));
    }
    m_textEditMainWnd->moveCursor(QTextCursor::End);
    m_centerWidget->setCurrentIndex(WndTextEdit);
    m_returnBtn->setVisible(true);
}

void VNoteMainWindow::onTextEditReturn()
{
    QString text = m_textEditMainWnd->toPlainText();
//    if (m_textNode->noteType == VNoteItem::VNOTE_TYPE::VNT_Text) {
//        if (text != m_textNode->metaData) {
//            if (!text.isEmpty()) {
//                m_textNode->metaData = text;
//                m_textEditRightView->setPlainText(text);
//                m_rightView->onUpdateNote(m_textNode);
//            } else {
//                m_rightView->noteDelFromCurFolder(m_textNode);
//            }
//        }
//    } else if (m_textNode->noteType == VNoteItem::VNT_Voice) {
//        if (m_isRecording == false && m_isAsrVoiceing == false) {
//            m_noteSearchEdit->setEnabled(true);
//        }
//    }
    m_centerWidget->setCurrentIndex(WndNoteShow);
    m_returnBtn->setVisible(false);
    onVNoteSearch();//详情页时只搜索详情页内容，返回时重新搜索
}

void VNoteMainWindow::onSearchNoteEmpty(qint64 id)
{
    //m_middleView->removeFromWhiteList(id);
}

void VNoteMainWindow::onStartRecord()
{
    m_rightView->insertVoiceItem();
}

void VNoteMainWindow::onFinshRecord(const QString &voicePath, qint64 voiceSize)
{
//    m_isRecording = false;
//    if (m_isAsrVoiceing == false) {
//        m_floatingAddNoteBtn->setBtnDisabled(false);
//        m_middleView->setFolderEnable(true);
//        m_noteSearchEdit->setEnabled(true);
//    }
//    m_rightView->addVoiceNoteItem(voicePath, voiceSize, m_isExit);
//    m_rightView->setVoicePlayEnable(true);

//    if (m_isExit) {
//        qApp->quit();
//    }

}

void VNoteMainWindow::onChangeTheme()
{
    DPalette pb = DApplicationHelper::instance()->palette(m_textEditMainWnd);
    pb.setBrush(DPalette::Text, pb.color(DPalette::Active, DPalette::WindowText));
    pb.setBrush(DPalette::Button, pb.color(DPalette::Base));
    m_textEditMainWnd->setPalette(pb);
}

void VNoteMainWindow::onA2TStart(const QString &file, qint64 duration)
{
//    m_isAsrVoiceing = true;
//    if (m_isRecording == false) {
//        m_floatingAddNoteBtn->setBtnDisabled(true);
//        m_middleView->setFolderEnable(false);
//        m_noteSearchEdit->setEnabled(false);
//    }
//    m_a2tManager->startAsr(file, duration);
}

void VNoteMainWindow::onA2TError(int error)
{
//    qInfo() << "Audo to text failed:" << error;
//    m_isAsrVoiceing = false;
//    if (m_isRecording == false) {
//        m_floatingAddNoteBtn->setBtnDisabled(false);
//        m_middleView->setFolderEnable(true);
//        m_noteSearchEdit->setEnabled(true);
//    }
//    m_rightView->setAsrResult("");
//    QString message = "";
//    if (error == VNoteA2TManager::NetworkError) {
//        message = DApplication::translate(
//                      "VNoteErrorMessage",
//                      "Network disconnected and cannot convert voice notes. Do you want to try again? ");
//    } else {
//        message = DApplication::translate(
//                      "VNoteErrorMessage",
//                      "The voice conversion failed. Do you want to try again?");
//    }
//    showAsrErrMessage(message);
}

void VNoteMainWindow::onA2TSuccess(const QString &text)
{
//    qInfo() << "Audo to text success:" << text;
//    m_isAsrVoiceing = false;
//    if (m_isRecording == false) {
//        m_floatingAddNoteBtn->setBtnDisabled(false);
//        m_middleView->setFolderEnable(true);
//        m_noteSearchEdit->setEnabled(true);
//    }
//    m_rightView->setAsrResult(text);
}

void VNoteMainWindow::initAsrErrMessage()
{
    m_asrErrMeassage = new DFloatingMessage(DFloatingMessage::ResidentType, m_rightViewHolder);
    m_asrErrMeassage->setIcon(QIcon(Utils::renderSVG(":/images/icon/normal/warning .svg", QSize(32, 32), qApp)));
    m_asrAgainBtn = new DPushButton(m_asrErrMeassage);
    m_asrAgainBtn->setText(tr("Try Again"));
    m_asrErrMeassage->setWidget(m_asrAgainBtn);
    m_asrErrMeassage->setVisible(false);
}

void VNoteMainWindow::showAsrErrMessage(const QString &strMessage)
{
    m_asrErrMeassage->setMessage(strMessage);
    m_asrErrMeassage->setFixedWidth(m_rightViewHolder->width());
    m_asrErrMeassage->adjustSize();
    m_asrErrMeassage->setVisible(true);
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
    if (m_asrErrMeassage->isVisible()) {
        m_asrErrMeassage->setFixedWidth(m_rightViewHolder->width());
    }
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

void VNoteMainWindow::onMenuNoteItemChange()
{
    if (m_asrErrMeassage->isVisible()) {
        m_asrErrMeassage->setVisible(false);
    }
}

void VNoteMainWindow::onAsrAgain()
{
//    m_asrErrMeassage->setVisible(false);
//    QTimer::singleShot(0, this, [this]() {
//        m_rightView->onAsrVoiceAction();
//    });
}

void VNoteMainWindow::onVNoteChange(const QModelIndex &previous)
{
    Q_UNUSED(previous);
    QModelIndex index = m_middleView->currentIndex();
    VNoteItem *data = static_cast<VNoteItem *>(StandardItemCommon::getStandardItemData(index));
    if(data){
        m_rightView->initData(data);
    }
    if(!m_searchKey.isEmpty() && m_noteSearchEdit->isEnabled()){
        m_noteSearchEdit->lineEdit()->setFocus();
    }
}
void VNoteMainWindow::onAction(QAction *action)
{
    ActionManager::ActionKind kind = ActionManager::getActionKind(action);
    switch (kind) {
    case ActionManager::NotepadRename:
        editNotepad();
        break;
    case ActionManager::NotepadDelete:
        delNotepad();
        break;
    case ActionManager::NotepadAddNew:
        addNote();
        break;
    case ActionManager::NoteDelete:
        delNote();
        break;
    case ActionManager::NoteAddNew:
        addNote();
        break;
    case ActionManager::NoteRename:
        editNote();
        break;
    default:
        break;
    }
}

int VNoteMainWindow::loadNotepads()
{
    VNOTE_FOLDERS_MAP *folders = VNoteDataManager::instance()->getNoteFolders();
    if (folders) {
        QList<QStandardItem *> items;
        folders->lock.lockForRead();
        for (auto it : folders->folders) {
            QStandardItem *pItem = StandardItemCommon::createStandardItem(it, StandardItemCommon::NOTEPADITEM);
            items.push_back(pItem);
        }
        folders->lock.unlock();
        if (items.size()) {
            QStandardItem *pItemRoot = m_leftView ->getNotepadRoot();
            if (pItemRoot) {
                pItemRoot->appendRows(items);
                QModelIndex index = m_leftView->setDefaultNotepadItem();
                onVNoteFolderChange(index, QModelIndex());
            }
            return  items.size();
        }
    }
    return 0;
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
        QStandardItem *pItem = StandardItemCommon::createStandardItem(newFolder, StandardItemCommon::NOTEPADITEM);
        QStandardItem *root = m_leftView->getNotepadRoot();
        root->insertRow(0, pItem);
        m_leftView->setCurrentIndex(pItem->index());
    }
}

void VNoteMainWindow::delNotepad()
{
    QModelIndex index = m_leftView->currentIndex();
    VNoteFolder *data = static_cast<VNoteFolder *>(StandardItemCommon::getStandardItemData(index));
    m_leftView->model()->removeRow(index.row(), index.parent());
    VNoteFolderOper  folderOper(data);
    folderOper.deleteVNoteFolder(data);
}

void VNoteMainWindow::editNotepad()
{
    QModelIndex index = m_leftView->currentIndex();
    m_leftView->edit(index);
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
        VNoteBlock* ptrBlock = tmpNote.datas.newBlock(VNoteBlock::Text);
        tmpNote.datas.addBlock(ptrBlock);

        static int id = 0;
        tmpNote.noteTitle = DApplication::translate("VNoteMainWindow", "Text") + QString::number(id++);
        VNoteItemOper noteOper;
        VNoteItem *newNote = noteOper.addNote(tmpNote);
        if (newNote) {
            QStandardItem *item = StandardItemCommon::createStandardItem(newNote, StandardItemCommon::NOTEITEM);
            m_middleView->getStandardItemModel()->insertRow(0, item);
            m_middleView->setCurrentIndex(item->index());
        }
    }
}

void VNoteMainWindow::editNote()
{
    QModelIndex index = m_middleView->currentIndex();
    m_middleView->edit(index);
}

void VNoteMainWindow::delNote()
{
    QModelIndex index = m_middleView->currentIndex();
    VNoteItem *noteData = static_cast< VNoteItem *>(StandardItemCommon::getStandardItemData(index));
    if (noteData) {
        m_middleView->getStandardItemModel()->removeRow(index.row());
        VNoteItemOper noteOper(noteData);
        noteOper.deleteNote();
    }
}
void VNoteMainWindow::loadNotes(qint64 id)
{
    QStandardItemModel *model = m_middleView->getStandardItemModel();
    model->removeRows(0, model->rowCount());
    m_middleView->setCurrentId(id);
    m_middleView->setVisibleEmptySearch(false);
    if (id != -1) {
        VNoteItemOper noteOper;
        VNOTE_ITEMS_MAP *notes = noteOper.getFolderNotes(id);
        if (notes) {
            notes->lock.lockForRead();
            for (auto it : notes->folderNotes) {
                QStandardItem *item = StandardItemCommon::createStandardItem(it, StandardItemCommon::NOTEITEM);
                model->appendRow(item);
            }
            notes->lock.unlock();
            QModelIndex index = model->index(0, 0);
            m_middleView->setCurrentIndex(index);
        }
    }
}

void VNoteMainWindow::loadSearchNotes(const QRegExp &key)
{
    QStandardItemModel *model = m_middleView->getStandardItemModel();
    model->removeRows(0, model->rowCount());
    m_middleView->setSearchKey(key);
    VNOTE_ALL_NOTES_MAP *noteAll = VNoteDataManager::instance()->getAllNotesInFolder();
    if (noteAll) {
        noteAll->lock.lockForRead();
        for (auto &it1 : noteAll->notes) {
            for (auto &it2 : it1->folderNotes) {
                if (it2->noteTitle.contains(key)
                        /*|| it2->noteText.contains(searchKey)*/) {
                    QStandardItem *item = StandardItemCommon::createStandardItem(it2, StandardItemCommon::NOTEITEM);
                    model->appendRow(item);
                }
            }
        }
        noteAll->lock.unlock();
        if(model->rowCount() == 0){
            m_middleView->setVisibleEmptySearch(true);
        }else {
            m_middleView->setVisibleEmptySearch(false);
            QModelIndex index = model->index(0, 0);
            m_middleView->setCurrentIndex(index);
        }
    }
}
