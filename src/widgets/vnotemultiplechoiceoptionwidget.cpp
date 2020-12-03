#include "vnotemultiplechoiceoptionwidget.h"

#include <libdtk-5.2.2/DWidget/dapplication.h>
//#include <DApplicationHelper>

//dx-多选详情页
VnoteMultipleChoiceOptionWidget::VnoteMultipleChoiceOptionWidget(QWidget *parent)
    :DWidget(parent)
{
    initUi();
}

/**
 * @brief VnoteMultipleChoiceOptionWidget::initUi
 * @param
 */
void VnoteMultipleChoiceOptionWidget::initUi()
{
    QVBoxLayout *vlayout = new QVBoxLayout();

    DLabel *iconLabel = new DLabel();
    QImage image(":/icons/deepin/multipleSelectPage/icon_notes.svg");
    iconLabel->setPixmap(QPixmap::fromImage(image));
    iconLabel->setFixedSize(186,186);
    QHBoxLayout *iconLayout = new QHBoxLayout();
    iconLayout->addStretch();
    iconLayout->addSpacing(30);
    iconLayout->addWidget(iconLabel,Qt::AlignHCenter);
    iconLayout->addStretch();

    m_tipsLabel = new DLabel();
//    m_tipsLabel->setText(DApplication::translate("%1 notes selected", "DetailPage").arg(m_noteNumber));
    m_tipsLabel->setFixedHeight(29);
    DFontSizeManager::instance()->bind(m_tipsLabel,DFontSizeManager::T4);
    QHBoxLayout *tipsLayout = new QHBoxLayout();
    tipsLayout->addStretch();
    tipsLayout->addWidget(m_tipsLabel);
    tipsLayout->addStretch();

    moveButton = new DToolButton();
    moveButton->setFixedHeight(26);
    moveButton->setText(DApplication::translate("NotesContextMenu", "Move"));
    moveButton->setIconSize(QSize(24,24));
    m_saveTextButton = new DToolButton();
    m_saveTextButton->setFixedHeight(26);
    m_saveTextButton->setText(DApplication::translate("NotesContextMenu", "Save as TXT"));
    m_saveTextButton->setIconSize(QSize(24,24));
    m_saveVoiceButton = new DToolButton();
    m_saveVoiceButton->setFixedHeight(26);
    m_saveVoiceButton->setText(DApplication::translate("NotesContextMenu", "Save voice recording"));
    m_saveVoiceButton->setIconSize(QSize(24,24));
    deleteButton = new DToolButton();
    deleteButton->setFixedHeight(26);
    deleteButton->setText(DApplication::translate("NotesContextMenu", "Delete"));
    deleteButton->setIconSize(QSize(24,24));
    //设置主题
    changeFromThemeType();
    //初始化链接
    initConnections();

    vlayout->addStretch(4);
    vlayout->addLayout(iconLayout);
    vlayout->addSpacing(10);
    vlayout->addLayout(tipsLayout);
    vlayout->addSpacing(20);
    QHBoxLayout *hlayout = new QHBoxLayout();
    hlayout->addStretch(1);
    hlayout->addWidget(moveButton);
    hlayout->addWidget(m_saveTextButton);
    hlayout->addWidget(m_saveVoiceButton);
    hlayout->addWidget(deleteButton);
    hlayout->addSpacing(10);
    hlayout->addStretch(1);
    vlayout->addLayout(hlayout);
    vlayout->addStretch(7);
    setLayout(vlayout);
}

/**
 * @brief VnoteMultipleChoiceOptionWidget::initConnections
 * @param number
 *///初始化链接
void VnoteMultipleChoiceOptionWidget::initConnections(){
    connect(moveButton,&DToolButton::clicked,this,[this]{
        trigger(ButtonValue::Move);
    });
    connect(m_saveTextButton,&DToolButton::clicked,this,[this]{
        trigger(ButtonValue::SaveAsTxT);
    });
    connect(m_saveVoiceButton,&DToolButton::clicked,this,[this]{
        trigger(ButtonValue::SaveAsVoice);
    });
    connect(deleteButton,&DToolButton::clicked,this,[this]{
        trigger(ButtonValue::Delete);
    });
    //主题切换更换按钮和文本颜色
    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this,
            &VnoteMultipleChoiceOptionWidget::changeFromThemeType);
}

/**
 * @brief VnoteMultipleChoiceOptionWidget::setNoteNumber
 * @param number
 *///设置笔记数量
void VnoteMultipleChoiceOptionWidget::setNoteNumber(int number)
{
    if(number != m_noteNumber){
        m_noteNumber = number;
        QString str = QString(DApplication::translate("DetailPage", "%1 notes selected").arg(number));
        m_tipsLabel->setText(str);
    }
}

/**
 * @brief VnoteMultipleChoiceOptionWidget::enableButtons
 * @param saveAsTxtButtonStatus,saveAsVoiceButtonStatus
 *///设置按钮是否置灰
void VnoteMultipleChoiceOptionWidget::enableButtons(bool saveAsTxtButtonStatus, bool saveAsVoiceButtonStatus)
{
    m_saveTextButton->setEnabled(saveAsTxtButtonStatus);
    m_saveVoiceButton->setEnabled(saveAsVoiceButtonStatus);
}

/**
 * @brief VnoteMultipleChoiceOptionWidget::trigger
 * @param id
 *///触发菜单操作
void VnoteMultipleChoiceOptionWidget::trigger(int id){
    emit requestMultipleOption(id);
}

/**
 * @brief VnoteMultipleChoiceOptionWidget::changeFromThemeType
 * @param
 *///dx-根据主题设置图标与删除按钮文本颜色
void VnoteMultipleChoiceOptionWidget::changeFromThemeType(){
    bool isDark = (DApplicationHelper::DarkType ==  DApplicationHelper::instance()->themeType())? true:false;
    if(isDark){
        moveButton->setIcon(QIcon(":/icons/deepin/multipleSelectPage/icon_move_dark.svg"));
        m_saveTextButton->setIcon(QIcon(":/icons/deepin/multipleSelectPage/icon_saveText_dark.svg"));
        m_saveVoiceButton->setIcon(QIcon(":/icons/deepin/multipleSelectPage/icon_saveVoice_dark.svg"));
        deleteButton->setIcon(QIcon(":/icons/deepin/multipleSelectPage/icon_delete_dark.svg"));
        //设置字体颜色（特殊颜色与UI沟通可以不根据DTK色板单独设置）
        DPalette deletePalette = DApplicationHelper::instance()->palette(deleteButton);
        deletePalette.setBrush(DPalette::ButtonText,QColor("#9A2F2F"));
        DApplicationHelper::instance()->setPalette(deleteButton, deletePalette);
    }else {
        moveButton->setIcon(QIcon(":/icons/deepin/multipleSelectPage/icon_move_light.svg"));
        m_saveTextButton->setIcon(QIcon(":/icons/deepin/multipleSelectPage/icon_saveText_light.svg"));
        m_saveVoiceButton->setIcon(QIcon(":/icons/deepin/multipleSelectPage/icon_saveVoice_light.svg"));
        deleteButton->setIcon(QIcon(":/icons/deepin/multipleSelectPage/icon_delete_light.svg"));
        //设置字体颜色（特殊颜色与UI沟通可以不根据DTK色板单独设置）
        DPalette deletePalette = DApplicationHelper::instance()->palette(deleteButton);
        deletePalette.setBrush(DPalette::ButtonText,QColor("#FF5736"));
        DApplicationHelper::instance()->setPalette(deleteButton, deletePalette);
    }
}
