#include "initemptypage.h"
#include "common/utils.h"

#include <QGridLayout>
#include <QVBoxLayout>

#include <DApplicationHelper>
#include <DFontSizeManager>

InitEmptyPage::InitEmptyPage(QWidget *parent)
    : DFrame(parent)
{
    initUi();
    initConnection();
}

void InitEmptyPage::onChangeColor()
{
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();

    DPalette pb = DApplicationHelper::instance()->palette(this);
    pb.setBrush(DPalette::Base, pb.color(DPalette::Window));
    this->setPalette(pb);
    if (nullptr != m_Text) {
        DPalette pa = DApplicationHelper::instance()->palette(m_Text);
        pa.setBrush(DPalette::WindowText, pa.color(DPalette::TextTips));
        m_Text->setPalette(pa);
    }
    if (nullptr != m_Image) {
        switch (themeType) {
            case DGuiApplicationHelper::LightType:
                m_Image->setPixmap(Utils::renderSVG(":/images/icon/icon_import_note.svg",
                                                    QSize(m_Image->width(), m_Image->height()),
                                                    qApp));
                break;
            case DGuiApplicationHelper::DarkType:
                m_Image->setPixmap(Utils::renderSVG(":/images/icon_dark/icon_import_note.svg",
                                                    QSize(m_Image->width(), m_Image->height()),
                                                    qApp));
                break;
            default:
                break;
        }
    }
}

void InitEmptyPage::initUi()
{
    m_PushButton = new DSuggestButton(QString(tr("Create Folder")), this);
    m_PushButton->setFixedSize(QSize(302, 36));
    DFontSizeManager::instance()->bind(m_PushButton, DFontSizeManager::T6);
    m_Image = new DLabel(this);
    m_Image->setFixedSize(QSize(128, 128));

    m_Text = new DLabel(this);
    m_Text->setFixedSize(QSize(500, 18));
    m_Text->setText(QString(tr("After create a folder, you can start your note")));
    m_Text->setAlignment(Qt::AlignCenter);
    DFontSizeManager::instance()->bind(m_Text, DFontSizeManager::T8);
    m_Text->setForegroundRole(DPalette::TextTips);
    onChangeColor();
    QGridLayout *layout = new QGridLayout;
    layout->setSpacing(15);
    layout->addWidget(m_Image, 1, 1, Qt::AlignCenter);
    layout->addWidget(m_PushButton, 2, 1, Qt::AlignCenter);
    layout->addWidget(m_Text, 3, 1, Qt::AlignCenter);
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 0);
    layout->setColumnStretch(2, 1);
    layout->setRowStretch(0, 1);
    layout->setRowStretch(1, 0);
    layout->setRowStretch(2, 0);
    layout->setRowStretch(3, 0);
    layout->setRowStretch(4, 1);
    this->setLayout(layout);
}

void InitEmptyPage::initConnection()
{
    connect(m_PushButton, SIGNAL(clicked()), this, SIGNAL(sigAddFolderByInitPage()));
    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this,
            &InitEmptyPage::onChangeColor);
}
