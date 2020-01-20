#include "views/vnotemainwindow.h"
#include "common/vnotedatamanager.h"
#include "common/vnoteaudiodevicewatcher.h"
#include "common/vnotea2tmanager.h"
#include "common/vnoteitem.h"
#include "common/utils.h"
#include "views/vnoterecordbar.h"

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

VNoteMainWindow::~VNoteMainWindow() {
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
            qInfo() << oldDbFile.fileName() << ":" <<oldDbFile.errorString();
        }
    }

    configDir.setFile(vnoteConfigBasePath+QDir::separator()+QString("config/"));

    if (!configDir.exists()) {
        QDir().mkpath(configDir.filePath());
        qInfo() << "create config dir:" << configDir.filePath();
    }

    m_qspSetting.reset(new QSettings(configDir.filePath()+QString("config.conf")
                                     ,QSettings::IniFormat));

}

void VNoteMainWindow::initConnections()
{
    connect(VNoteDataManager::instance(), &VNoteDataManager::onNoteFoldersLoaded,
            this, &VNoteMainWindow::onVNoteFoldersLoaded);

    connect(m_floatingAddBtn, &DFloatingButton::clicked,
            m_leftView, &LeftView::handleAddFolder);

    connect(m_noteSearchEdit, &DSearchEdit::textChanged,
            this, &VNoteMainWindow::onVNoteSearch);

    connect(m_leftView, SIGNAL(currentChanged(const QModelIndex &)),
            this, SLOT(onVNoteFolderChange(const QModelIndex &)));
    connect(m_leftView, &LeftView::sigFolderDel,
            this, &VNoteMainWindow::onVNoteFolderDel);

    connect(m_wndHomePage, &InitEmptyPage::sigAddFolderByInitPage,
            this, &VNoteMainWindow::onVNoteFolderAdd);

    connect(m_returnBtn, &DIconButton::clicked,
            this, &VNoteMainWindow::onTextEditReturn);

    connect(m_rightView, &RightView::sigTextEditDetail,
            this, &VNoteMainWindow::onTextEditDetail);
    connect(m_rightView, &RightView::sigSearchNoteEmpty,
            this, &VNoteMainWindow::onSearchNoteEmpty);
    connect(m_rightView, &RightView::sigMenuNoteItemChange,
            this, &VNoteMainWindow::onMenuNoteItemChange);

    connect(m_recordBar, &VNoteRecordBar::sigStartRecord, this, &VNoteMainWindow::onStartRecord);
    connect(m_recordBar, &VNoteRecordBar::sigFinshRecord, this, &VNoteMainWindow::onFinshRecord);

    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this,
            &VNoteMainWindow::onChangeTheme);

    connect(m_asrAgainBtn, &DPushButton::clicked, this, &VNoteMainWindow::onAsrAgain);

}

void VNoteMainWindow::initShortcuts() {
    m_stPreviewShortcuts.reset(new QShortcut(this));
    m_stPreviewShortcuts->setKey(tr("Ctrl+Shift+/"));
    m_stPreviewShortcuts->setContext(Qt::ApplicationShortcut);
    m_stPreviewShortcuts->setAutoRepeat(false);

    connect(m_stPreviewShortcuts.get(), &QShortcut::activated,
            this, &VNoteMainWindow::onPreviewShortcut);
}

void VNoteMainWindow::initTitleBar()
{
    titlebar()->setFixedHeight(VNOTE_TITLEBAR_HEIGHT);
    // Add logo
    titlebar()->setIcon(QIcon::fromTheme(DEEPIN_VOICE_NOTE));
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

    QVBoxLayout *leftHolderLayout = new  QVBoxLayout();
    leftHolderLayout->setSpacing(0);
    leftHolderLayout->setContentsMargins(0, 0, 0, 0);

    m_leftView = new LeftView(this);

    m_leftView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_leftView->setBackgroundRole(DPalette::Base);
    m_leftView->setSpacing(5);
    m_leftView->setAutoFillBackground(true);
    m_leftView->setContentsMargins(0, 5, 0, 0);

    m_floatingAddBtn = new DFloatingButton(DStyle::SP_IncreaseElement, m_leftView);
    m_floatingAddBtn->setFixedSize(QSize(55, 55));

    DAnchorsBase buttonAnchor(m_floatingAddBtn);
    buttonAnchor.setAnchor(Qt::AnchorLeft, m_leftView, Qt::AnchorLeft);
    buttonAnchor.setAnchor(Qt::AnchorBottom, m_leftView, Qt::AnchorBottom);
    buttonAnchor.setBottomMargin(11);
    buttonAnchor.setLeftMargin(97);

    // ToDo:
    //    Add Left view widget here

    leftHolderLayout->addWidget(m_leftView);

    m_leftViewHolder->setLayout(leftHolderLayout);

#ifdef VNOTE_LAYOUT_DEBUG
    m_leftViewHolder->setStyleSheet("background: green");
#endif
}

void VNoteMainWindow::initRightView()
{
    m_rightViewHolder = new QWidget(m_mainWndSpliter);
    m_rightViewHolder->setObjectName("rightMainLayoutHolder");
    m_rightViewHolder->setSizePolicy(QSizePolicy::Expanding
                                     , QSizePolicy::Expanding);

    QVBoxLayout *rightHolderLayout = new QVBoxLayout(m_rightViewHolder);
    rightHolderLayout->setSpacing(0);
    rightHolderLayout->setContentsMargins(0, 0, 0, 0);

    //TODO:
    //    Add note area code here
    m_rightNoteArea = new QWidget(m_rightViewHolder);
    m_rightNoteArea->setBackgroundRole(DPalette::Base);
    m_rightNoteArea->setAutoFillBackground(true);

    QVBoxLayout *rightNoteAreaLayout = new QVBoxLayout(m_rightNoteArea);
    rightNoteAreaLayout->setSpacing(0);
    rightNoteAreaLayout->setContentsMargins(0, 0, 0, 0);

    m_rightView = new RightView(m_rightViewHolder);
    m_rightView->setSizePolicy(QSizePolicy::Preferred
                               , QSizePolicy::Preferred);
    //m_rightView->setBackgroundRole(DPalette::Base);
    //m_rightView->setAutoFillBackground(true);

    rightNoteAreaLayout->addWidget(m_rightView);

    //TODO:
    //    Add record area code here
    m_recordBar = new VNoteRecordBar(m_rightViewHolder);
    m_recordBar->setBackgroundRole(DPalette::Base);
    m_recordBar->setAutoFillBackground(true);
    m_recordBar->setFixedHeight(78);
    m_recordBar->setSizePolicy(QSizePolicy::Expanding
                               , QSizePolicy::Fixed);

    rightHolderLayout->addWidget(m_rightNoteArea);
    rightHolderLayout->addWidget(m_recordBar);

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

        connect(m_rightView, &RightView::asrStart, this, &VNoteMainWindow::onA2TStart);
        connect(m_a2tManager, &VNoteA2TManager::asrError, this, &VNoteMainWindow::onA2TError);
        connect(m_a2tManager, &VNoteA2TManager::asrSuccess, this, &VNoteMainWindow::onA2TSuccess);
    });
}

void VNoteMainWindow::onVNoteFoldersLoaded()
{
    int count = m_leftView->loadNoteFolder(); //加载完成前都是显示主页
    if (!count) {
        m_centerWidget->setCurrentIndex(WndHomePage);
        m_noteSearchEdit->setEnabled(false);
    }
}

void VNoteMainWindow::onVNoteSearch()
{
    QString strKey = m_noteSearchEdit->text();
    QRegExp regExp;
    WindowType type = static_cast<WindowType>(m_centerWidget->currentIndex());
    if (!strKey.isEmpty()) {
        regExp.setPattern(strKey);
        regExp.setCaseSensitivity(Qt::CaseInsensitive);
    }
    if (type == WndTextEdit) { //详情页时只搜索详情页内容
        QString text = m_textEditMainWnd->toPlainText();
        m_textEditMainWnd->setCurrentCharFormat(m_textEditFormat);
        m_textEditMainWnd->setText(text);
        Utils::highTextEdit(m_textEditMainWnd, m_textEditFormat, regExp, QColor(0x349ae8));
    } else {
        m_rightView->setSearchKey(regExp);
        if (regExp.isEmpty()) {
            m_leftView->clearFilter();
        } else {
            QList<qint64> folders = m_rightView->getNoteContainsKeyFolders(regExp);
            m_leftView->setFolderNameFilter(regExp, &folders);
        }
    }
}

void VNoteMainWindow::onVNoteFolderChange(const QModelIndex &previous)
{
    Q_UNUSED(previous);
    bool foucsFlag = m_noteSearchEdit->lineEdit()->hasFocus();
    QModelIndex index = m_leftView->currentIndex();
    if (index.isValid()) {
        m_leftView->update();
        qint64 id = m_leftView->getFolderId(index);
        m_rightView->noteSwitchByFolder(id);
        m_centerWidget->setCurrentIndex(WndNoteShow);
    } else {
        m_centerWidget->setCurrentIndex(WndSearchEmpty);
    }
    if(foucsFlag){
       m_noteSearchEdit->lineEdit()->setFocus();
    }
    if(m_asrErrMeassage && m_asrErrMeassage->isVisible()){
        m_asrErrMeassage->setVisible(false);
    }
}

void VNoteMainWindow::onVNoteFolderDel(VNoteFolder *data)
{
    m_rightView->noteDelByFolder(data->id);
    VNoteFolderOper folderOper(data);
    folderOper.deleteVNoteFolder(data->id);
    if (m_leftView->getFolderCount() == 0) {
        m_centerWidget->setCurrentIndex(WndHomePage);
        m_noteSearchEdit->setEnabled(false);
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
    m_labSearchEmpty->setText(QString(tr("No Result")));
    DFontSizeManager::instance()->bind(m_labSearchEmpty, DFontSizeManager::T4);
    m_labSearchEmpty->setAlignment(Qt::AlignCenter);
    m_labSearchEmpty->setFocusPolicy(Qt::NoFocus);
}

void VNoteMainWindow::initSpliterView()
{
    m_mainWndSpliter = new DSplitter(Qt::Horizontal, this);

    initLeftView();
    initRightView();

    // Disable spliter drag & resize
    QSplitterHandle *handle = m_mainWndSpliter->handle(1);
    if (handle) {
        handle->setFixedWidth(2);
        handle->setDisabled(true);

        DPalette pa = DApplicationHelper::instance()->palette(handle);
        QBrush splitBrush = pa.brush(DPalette::ItemBackground);
        pa.setBrush(DPalette::Background, splitBrush);
        handle->setPalette(pa);
        handle->setBackgroundRole(QPalette::Background);
        handle->setAutoFillBackground(true);
    }
}

void VNoteMainWindow::initEmptyFoldersView()
{
    m_wndHomePage = new InitEmptyPage(this);
}

void VNoteMainWindow::onVNoteFolderAdd()
{
    m_centerWidget->setCurrentIndex(WndNoteShow);
    m_noteSearchEdit->setEnabled(true);
    m_floatingAddBtn->clicked();
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
    if (m_textNode->noteType == VNoteItem::VNOTE_TYPE::VNT_Text) {
        if (text != m_textNode->noteText) {
            m_textNode->noteText = text;
            m_textEditRightView->setPlainText(text);
            m_rightView->onUpdateNote(m_textNode);
        }
    } else if (m_textNode->noteType == VNoteItem::VNT_Voice) {
        if (m_isRecording == false && m_isAsrVoiceing == false) {
            m_noteSearchEdit->setEnabled(true);
        }
    }
    m_centerWidget->setCurrentIndex(WndNoteShow);
    m_returnBtn->setVisible(false);
    onVNoteSearch();//详情页时只搜索详情页内容，返回时重新搜索
}

void VNoteMainWindow::onSearchNoteEmpty(qint64 id)
{
    m_leftView->removeFromWhiteList(id);
}

void VNoteMainWindow::onStartRecord()
{
    if(m_isAsrVoiceing == false){
        m_leftView->setFolderEnable(false);
        m_noteSearchEdit->setEnabled(false);
    }
    m_rightView->setVoicePlayEnable(false);
    m_isRecording = true;
}

void VNoteMainWindow::onFinshRecord(const QString &voicePath, qint64 voiceSize)
{
    m_isRecording = false;
    if(m_isAsrVoiceing == false){
        m_leftView->setFolderEnable(true);
        m_noteSearchEdit->setEnabled(true);
    }
    m_rightView->addVoiceNoteItem(voicePath, voiceSize);
    m_rightView->setVoicePlayEnable(true);

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
    m_isAsrVoiceing = true;
    if(m_isRecording == false){
        m_leftView->setFolderEnable(false);
        m_noteSearchEdit->setEnabled(false);
    }
    m_a2tManager->startAsr(file, duration);
}

void VNoteMainWindow::onA2TError(int error)
{
    qInfo() << "Audo to text failed:" << error;
    m_isAsrVoiceing = false;
    if(m_isRecording == false){
        m_leftView->setFolderEnable(true);
        m_noteSearchEdit->setEnabled(true);
    }
    m_rightView->setAsrResult("");
    QString message = "";
    if(error == VNoteA2TManager::NetworkError){
        message = tr("The voice conversion failed due to the poor network connection. Do you want to try again?");
    }else {
        message = tr("The voice conversion failed. Do you want to try again? ");
    }
    showAsrErrMessage(message);
}

void VNoteMainWindow::onA2TSuccess(const QString &text)
{
    qInfo() << "Audo to text success:" << text;
    m_isAsrVoiceing = false;
    if(m_isRecording == false){
        m_leftView->setFolderEnable(true);
        m_noteSearchEdit->setEnabled(true);
    }
    m_rightView->setAsrResult(text);
}

void VNoteMainWindow::initAsrErrMessage()
{
    m_asrErrMeassage = new DFloatingMessage(DFloatingMessage::ResidentType,m_rightViewHolder);
    m_asrErrMeassage->setIcon(QIcon(Utils::renderSVG(":/images/icon/normal/warning .svg", QSize(32,32),qApp)));
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
        {"Help",                 "F1"},
//        {"Close window",         "Alt+F4"},
        {"Display shortcuts",    "Ctrl+Shift+?"},
        {"New notebook",         "Ctrl+N"},
        {"Delete notebook/note", "Delete"},
//        {"Resize window",        "Ctrl+Alt+F"},
//        {"Find",                 "Ctrl+F"},
        {"Rename notebook",      "F2"},
        {"Play/Pause",           "Space"},
        {"Select all",           "Ctrl+A"},
        {"Copy",                 "Ctrl+C"},
        {"Cut",                  "Ctrl+X"},
        {"Paste",                "Ctrl+V"},
    };

    QJsonObject fontMgrJsonGroup;
    fontMgrJsonGroup.insert("groupName", DApplication::translate("AppMain", "Voice Notes"));
    QJsonArray fontJsonItems;

    for (QMap<QString, QString>::iterator it = shortcutKeymap.begin();
         it != shortcutKeymap.end(); it++) {
        QJsonObject jsonItem;
        jsonItem.insert("name", DApplication::translate("Shortcuts", it.key().toUtf8()));
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
    if(m_asrErrMeassage->isVisible()){
        m_asrErrMeassage->setFixedWidth(m_rightViewHolder->width());
    }
    DMainWindow::resizeEvent(event);
}

void VNoteMainWindow::onMenuNoteItemChange()
{
    if(m_asrErrMeassage->isVisible()){
        m_asrErrMeassage->setVisible(false);
    }
}

void VNoteMainWindow::onAsrAgain()
{
    m_asrErrMeassage->setVisible(false);
    QTimer::singleShot(0, this, [this]() {
        m_rightView->onAsrVoiceAction();
    });
}
