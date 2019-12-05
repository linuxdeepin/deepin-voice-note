#include "views/vnotemainwindow.h"
#include "common/vnotedatamanager.h"
#include "globaldef.h"

#include <DTitlebar>
#include <DStatusBar>
#include <DApplication>
#include <DFontSizeManager>
#include <DApplicationHelper>
#include <DLog>

VNoteMainWindow::VNoteMainWindow(QWidget *parent)
    :DMainWindow (parent)
{
    initUI();
    initConnections();

    //Request DataManager load  note folders
    initData();
}

VNoteMainWindow::~VNoteMainWindow()
{

}

void VNoteMainWindow::initUI()
{
    initTitleBar();
    initMainView();
}

void VNoteMainWindow::initData()
{
    VNoteDataManager::instance()->reqNoteFolders();
}

void VNoteMainWindow::initConnections()
{
    connect(VNoteDataManager::instance(), &VNoteDataManager::onNoteFoldersLoaded
            , this, &VNoteMainWindow::onVNoteFoldersLoaded);
    connect(m_btnAddFoler, &DPushButton::clicked, m_leftViewHolder, &LeftView::handleAddFolder);
}

void VNoteMainWindow::initShortcuts()
{

}

void VNoteMainWindow::initTitleBar()
{
    titlebar()->setFixedHeight(VNOTE_TITLEBAR_HEIGHT);
    //Add logo
    titlebar()->setIcon(QIcon::fromTheme(DEEPIN_VOICE_NOTE));
    // Search note
    m_noteSearchEdit = new DSearchEdit(this);
    DFontSizeManager::instance()->bind(m_noteSearchEdit, DFontSizeManager::T6);
    m_noteSearchEdit->setFixedSize(QSize(VNOTE_SEARCHBAR_W, VNOTE_SEARCHBAR_H));
    m_noteSearchEdit->setPlaceHolder(DApplication::translate("TitleBar", "Search"));

    titlebar()->addWidget(m_noteSearchEdit);
}

void VNoteMainWindow::initMainView()
{
    m_mainWndSpliter = new DSplitter(Qt::Horizontal, this);

    initLeftView();
    initRightView();

    //Disable spliter drag & resize
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

    //Enable drag resize window
    //statusBar()->setSizeGripEnabled(true);

    setCentralWidget(m_mainWndSpliter);
}

void VNoteMainWindow::initLeftView()
{
    m_leftViewHolder = new LeftView(m_mainWndSpliter);
    m_leftViewHolder->setObjectName("leftMainLayoutHolder");
    m_leftViewHolder->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    m_leftViewHolder->setFixedWidth(VNOTE_LEFTVIEW_W);
    m_leftViewHolder->setContentsMargins(0, 0, 2, 5);
    m_leftViewHolder->setBackgroundRole(DPalette::Base);
    m_leftViewHolder->setAutoFillBackground(true);
    m_leftViewHolder->setSpacing(5);
    // d->leftBarHolder->setAttribute(Qt::WA_TranslucentBackground, true);
    m_btnAddFoler = new DPushButton (m_leftViewHolder);
    m_btnAddFoler->setFixedSize(QSize(68, 68));

    QVBoxLayout *leftMainLayout = new QVBoxLayout();
    leftMainLayout->setContentsMargins(0, 0, 0, 0);
    leftMainLayout->setSpacing(0);

    // ToDo:
    //    Add Left view widget here

    m_leftViewHolder->setLayout(leftMainLayout);
#ifdef VNOTE_LAYOUT_DEBUG
    m_leftViewHolder->setStyleSheet("background: green");
#endif
}

void VNoteMainWindow::initRightView()
{
    m_rightViewHolder = new QWidget(m_mainWndSpliter);
    m_rightViewHolder->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_rightViewHolder->setObjectName("rightMainLayoutHolder");
    m_rightViewHolder->setBackgroundRole(DPalette::Base);
    m_rightViewHolder->setAutoFillBackground(true);

    QVBoxLayout *rightMainLayout = new QVBoxLayout();
    rightMainLayout->setContentsMargins(0, 0, 0, 0);
    rightMainLayout->setSpacing(0);

    //TODO:
    //    Add Right view code

    m_rightViewHolder->setLayout(rightMainLayout);
#ifdef VNOTE_LAYOUT_DEBUG
    m_rightViewHolder->setStyleSheet("background: red");
#endif
}

void VNoteMainWindow::onVNoteFoldersLoaded()
{
    //qInfo() << "folderMaps:" << VNoteDataManager::instance()->getNoteFolders();
    m_leftViewHolder->loadNoteFolder();
}
void VNoteMainWindow::resizeEvent(QResizeEvent *event)
{
    m_btnAddFoler->move((m_leftViewHolder->width() - m_btnAddFoler->width()) / 2,
                        this->height() - m_btnAddFoler->height() - 60);
    DMainWindow::resizeEvent(event);
}
