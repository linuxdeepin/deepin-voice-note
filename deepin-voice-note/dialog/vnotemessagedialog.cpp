#include "vnotemessagedialog.h"

#include <QVBoxLayout>

#include <DVerticalLine>
#include <DApplication>
#include <DFontSizeManager>
#include <DApplicationHelper>

VNoteMessageDialog::VNoteMessageDialog(int msgType, QWidget *parent)
    : VNoteBaseDialog(parent)
    , m_msgType(static_cast<MessageType>(msgType))
{
    initUI();
    initConnections();
    initMessage();
}

void VNoteMessageDialog::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(10, 0, 10, 10);

    QWidget *mainFrame = new QWidget(this);
    mainFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_pMessage = new DLabel(this);
    m_pMessage->setWordWrap(true);
    DFontSizeManager::instance()->bind(m_pMessage, DFontSizeManager::T6);

    //Set Message color to TextTips
    DPalette paMsg = DApplicationHelper::instance()->palette(m_pMessage);
    paMsg.setBrush(DPalette::WindowText, paMsg.color(DPalette::TextTips));
    m_pMessage->setPalette(paMsg);

    QHBoxLayout *actionBarLayout = new QHBoxLayout();
    actionBarLayout->setSpacing(0);
    actionBarLayout->setContentsMargins(0, 0, 0, 0);

    m_cancelBtn  = new DPushButton(this);
    m_confirmBtn = new DWarningButton(this);

    DVerticalLine *verticalSplite = new DVerticalLine(this);
    DPalette pa = DApplicationHelper::instance()->palette(verticalSplite);
    QColor splitColor = pa.color(DPalette::ItemBackground);
    pa.setBrush(DPalette::Background, splitColor);
    verticalSplite->setPalette(pa);
    verticalSplite->setBackgroundRole(QPalette::Background);
    verticalSplite->setAutoFillBackground(true);
    verticalSplite->setFixedSize(3,28);

    actionBarLayout->addWidget(m_cancelBtn);
    actionBarLayout->addSpacing(8);
    actionBarLayout->addWidget(verticalSplite);
    actionBarLayout->addSpacing(8);
    actionBarLayout->addWidget(m_confirmBtn);

    mainLayout->addWidget(m_pMessage, 1, Qt::AlignTop|Qt::AlignCenter);
    mainLayout->addStretch();
    mainLayout->addLayout(actionBarLayout);
    mainFrame->setLayout(mainLayout);

    addContent(mainFrame);
}

void VNoteMessageDialog::initConnections()
{
    connect(m_cancelBtn, &DPushButton::clicked
            , this, [=]() {
        this->reject();
    });

    connect(m_confirmBtn, &DPushButton::clicked
            , this, [=]() {
        this->accept();
    });
}

void VNoteMessageDialog::initMessage()
{
    //TODO:
    //   The default button text is Cancel & OK
    //In some case OK button text is need to change
    m_cancelBtn->setText(DApplication::translate("VNoteMessageDialog", "Cancel"));
    m_confirmBtn->setText(DApplication::translate("VNoteMessageDialog", "OK"));

    switch (m_msgType) {
    case DeleteNote: {
        m_pMessage->setText(DApplication::translate("VNoteMessageDialog"
            ,"Are you sure want to delete Folder?"));
        m_confirmBtn->setText(DApplication::translate("VNoteMessageDialog", "Delete"));
    } break;
    case AbortRecord: {
        m_pMessage->setText(DApplication::translate("VNoteMessageDialog"
            ,"Currently recording,is the recording terminated?"));
    } break;
    }
}
