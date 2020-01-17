#include "views/vnotemainwindow.h"
#include "common/vnotedatamanager.h"
#include "common/vnoteaudiodevicewatcher.h"
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

    // Request DataManager load  note folders
    initData();

    //Start audio device watch thread
    //& must be called after initUI
    initAudioWatcher();
}

VNoteMainWindow::~VNoteMainWindow() {
    //TODO:
    //    Nofiy audio watcher to exit & need
    //wait at least AUDIO_DEV_CHECK_TIME ms
    QScopedPointer<VNoteAudioDeviceWatcher> autoRelease(
                m_audioDeviceWatcher
                );
    autoRelease->exitWatcher();
    autoRelease->wait(AUDIO_DEV_CHECK_TIME);
}

void VNoteMainWindow::initUI()
{
    initTitleBar();
    initMainView();
}

void VNoteMainWindow::initData()
{
    VNoteDataManager::instance()->reqNoteDefIcons();
    VNoteDataManager::instance()->reqNoteFolders();
    VNoteDataManager::instance()->reqNoteItems();
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
    connect(m_rightView, &RightView::sigSeachEditFocus,
            this, &VNoteMainWindow::onSearchEditFocus);
    connect(m_rightView, &RightView::sigSearchNoteEmpty,
            this, &VNoteMainWindow::onSearchNoteEmpty);

    connect(m_recordBar, &VNoteRecordBar::sigStartRecord, this, &VNoteMainWindow::onStartRecord);
    connect(m_recordBar, &VNoteRecordBar::sigFinshRecord, this, &VNoteMainWindow::onFinshRecord);

    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this,
            &VNoteMainWindow::onChangeTheme);

}

void VNoteMainWindow::initShortcuts() {}

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
        m_audioDeviceWatcher = new VNoteAudioDeviceWatcher(this);

        connect(m_audioDeviceWatcher, &VNoteAudioDeviceWatcher::microphoneAvailableState,
                m_recordBar, &VNoteRecordBar::OnMicrophoneAvailableChanged);

        m_audioDeviceWatcher->start();
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
    QModelIndex index = m_leftView->currentIndex();
    if (index.isValid()) {
        qint64 id = m_leftView->getFolderId(index);
        m_rightView->noteSwitchByFolder(id);
        m_centerWidget->setCurrentIndex(WndNoteShow);
    } else {
        m_centerWidget->setCurrentIndex(WndSearchEmpty);
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
        if (m_isRecording == false) {
            m_noteSearchEdit->setEnabled(true);
        }
    }
    m_centerWidget->setCurrentIndex(WndNoteShow);
    m_returnBtn->setVisible(false);
    onVNoteSearch();//详情页时只搜索详情页内容，返回时重新搜索
}

void VNoteMainWindow::onSearchEditFocus()
{
    m_noteSearchEdit->lineEdit()->setFocus();
}

void VNoteMainWindow::onSearchNoteEmpty(qint64 id)
{
    m_leftView->removeFromWhiteList(id);
}

void VNoteMainWindow::onStartRecord()
{
    m_leftView->setEnabled(false);
    m_noteSearchEdit->setEnabled(false);
    m_rightView->setVoicePlayEnable(false);
    m_isRecording = true;
}

void VNoteMainWindow::onFinshRecord(const QString &voicePath, qint64 voiceSize)
{
    m_leftView->setEnabled(true);
    m_rightView->addVoiceNoteItem(voicePath, voiceSize);
    m_noteSearchEdit->setEnabled(true);
    m_rightView->setVoicePlayEnable(true);
    m_isRecording = false;
}

void VNoteMainWindow::onChangeTheme()
{
    DPalette pb = DApplicationHelper::instance()->palette(m_textEditMainWnd);
    pb.setBrush(DPalette::Text, pb.color(DPalette::Active, DPalette::WindowText));
    pb.setBrush(DPalette::Button, pb.color(DPalette::Base));
    m_textEditMainWnd->setPalette(pb);
}
