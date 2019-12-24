#include "views/vnotemainwindow.h"
#include "common/vnotedatamanager.h"
#include "common/vnoteitem.h"
#include "common/utils.h"

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
}

VNoteMainWindow::~VNoteMainWindow() {}

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
    connect(VNoteDataManager::instance(), &VNoteDataManager::onNoteFoldersLoaded, this,
            &VNoteMainWindow::onVNoteFoldersLoaded);
    connect(m_floatingAddBtn, &DFloatingButton::clicked, m_leftView, &LeftView::handleAddFolder);
    connect(m_noteSearchEdit, &DSearchEdit::textChanged, this, &VNoteMainWindow::onVNoteSearch);
    connect(m_leftView, SIGNAL(currentChanged(const QModelIndex &)), this,
            SLOT(onVNoteFolderChange(const QModelIndex &)));
    connect(m_leftView, &LeftView::sigFolderDel, this, &VNoteMainWindow::onVNoteFolderDel);
    connect(m_wndHomePage, &InitEmptyPage::sigAddFolderByInitPage, this,
            &VNoteMainWindow::onVNoteFolderAdd);
    connect(m_rightViewHolder, &RightView::sigTextEditDetail, this,
            &VNoteMainWindow::onTextEditDetail);
    connect(m_returnBtn, &DIconButton::clicked, this, &VNoteMainWindow::onTextEditReturn);
    connect(m_rightViewHolder, &RightView::sigSeachEditFocus, this, &VNoteMainWindow::onSearchEditFocus);
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
    m_noteSearchEdit->setEnabled(false);
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
    switchView(WndNoteShow);
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
    m_rightViewHolder = new RightView(m_mainWndSpliter);
    m_rightViewHolder->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_rightViewHolder->setObjectName("rightMainLayoutHolder");
    m_rightViewHolder->setBackgroundRole(DPalette::Base);
    m_rightViewHolder->setAutoFillBackground(true);
#ifdef VNOTE_LAYOUT_DEBUG
    m_rightViewHolder->setStyleSheet("background: red");
#endif
}

void VNoteMainWindow::onVNoteFoldersLoaded()
{
    int count = m_leftView->loadNoteFolder(); //加载完成前都是显示主页
    if (!count) {
        switchView(WndHomePage);
    }
}

void VNoteMainWindow::onVNoteSearch()
{
    QString strKey = m_noteSearchEdit->text();
    m_rightViewHolder->setSearchKey(strKey);
    if (!strKey.isEmpty()) {
        QList<qint64> folders = m_rightViewHolder->getNoteContainsKeyFolders(strKey);
        m_leftView->setFolderNameFilter(strKey, &folders);
        qDebug() << "id size:" << folders;
    } else {
        m_leftView->clearFilter();
        switchView(WndNoteShow);
    }
}

void VNoteMainWindow::onVNoteFolderChange(const QModelIndex &previous)
{
    Q_UNUSED(previous);
    QModelIndex index = m_leftView->currentIndex();
    if (index.isValid()) {
        qint64 id = m_leftView->getFolderId(index);
        m_rightViewHolder->noteSwitchByFolder(id);
        switchView(WndNoteShow);
    } else {
        switchView(WndSearchEmpty);
    }
}

void VNoteMainWindow::onVNoteFolderDel(VNoteFolder *data)
{
    m_rightViewHolder->noteDelByFolder(data->id);
    VNoteFolderOper folderOper(data);
    folderOper.deleteVNoteFolder(data->id);
    if (m_leftView->getFolderCount() == 0) {
        switchView(WndHomePage);
        m_noteSearchEdit->setEnabled(false);
    }
}

void VNoteMainWindow::initTextEditDetailView()
{
    m_textEditMainWnd = new DTextEdit(this);
    DFontSizeManager::instance()->bind(m_textEditMainWnd, DFontSizeManager::T8);
    m_textEditFormat = m_textEditMainWnd->currentCharFormat();
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

void VNoteMainWindow::switchView(WindowType type)
{
    m_noteSearchEdit->setEnabled(type == WndNoteShow || type == WndSearchEmpty);
    m_centerWidget->setCurrentIndex(type);
}

void VNoteMainWindow::onVNoteFolderAdd()
{
    switchView(WndNoteShow);
    m_floatingAddBtn->clicked();
}

void VNoteMainWindow::onTextEditDetail(VNoteItem *textNode, DTextEdit *preTextEdit, const QString &searchKey)
{
    m_textNode = textNode;
    m_textEditRightView = preTextEdit;
    QTextCursor cursorSrc = preTextEdit->textCursor();
    m_textEditMainWnd->setText(textNode->noteText);
    if (!searchKey.isEmpty()) {
        Utils::highTextEdit(m_textEditMainWnd, &m_textEditFormat, searchKey, Qt::red);
    }
    QTextCursor cursordst = m_textEditMainWnd->textCursor();
    cursordst.setPosition(cursorSrc.position());
    m_textEditMainWnd->setTextCursor(cursordst);
    switchView(WndTextEdit);
    m_returnBtn->setVisible(true);
}

void VNoteMainWindow::onTextEditReturn()
{
    QString text = m_textEditMainWnd->toPlainText();
    if (text != m_textNode->noteText) {
        m_textNode->noteText = text;
        m_textEditRightView->setText(text);
        m_rightViewHolder->onUpdateNote(m_textNode);
    }
    switchView(WndNoteShow);
    m_returnBtn->setVisible(false);
}

void VNoteMainWindow::onSearchEditFocus()
{
    m_noteSearchEdit->lineEdit()->setFocus();
}
