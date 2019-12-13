#include "rightview.h"
#include "common/vnoteitem.h"
#include "rightnotelist.h"
#include "common/vnotedatamanager.h"
#include <QBoxLayout>
#include <QDebug>

#include <DFontSizeManager>
#include <DGuiApplicationHelper>

RightView::RightView(QWidget *parent)
    : QWidget(parent)
{
    initUi();
    initConnection();
}

void RightView::initUi()
{
    m_stackWidget = new DStackedWidget(this);
    m_stackWidget->setContentsMargins(0, 0, 0, 0);

    QVBoxLayout *layoutList = new QVBoxLayout;
    layoutList->setContentsMargins(0, 0, 0, 0);
    layoutList->addWidget(m_stackWidget);
    layoutList->addSpacing(80);

    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if(themeType == DGuiApplicationHelper::LightType)
    {
        m_addVoiceBtn = new MyRecodeButtons(
                    ":/images/icon/normal/circlebutton_voice.svg",
                    ":/images/icon/press/circlebutton_voice_press.svg",
                    ":/images/icon/hover/circlebutton_voice_hover.svg",
                    ":/images/icon/disabled/circlebutton_voice_disabled.svg",
                    ":/images/icon/focus/circlebutton_voice_focus.svg",
                    QSize(68,68),
                    this);
    }
    else
    {
        m_addVoiceBtn = new MyRecodeButtons(
                    ":/images/icon_dark/normal/voice_normal_dark.svg",
                    ":/images/icon_dark/press/voice_press_dark.svg",
                    ":/images/icon_dark/hover/voice_hover_dark.svg",
                    ":/images/icon_dark/disabled/voice_disabled_dark.svg",
                    ":/images/icon_dark/focus/voice_focus_dark.svg",
                    QSize(68,68),
                    this);
    }
    QGridLayout *layout = new QGridLayout;
    layout->setSpacing(0);
    layout->setContentsMargins(0,0,0,0);
    layout->addLayout(layoutList,0,0);
    layout->addWidget(m_addVoiceBtn,1,0,Qt::AlignHCenter);
    this->setLayout(layout);
    m_searchNoteList = new RightNoteList(this);
    m_searchNoteList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_searchNoteList->setLineWidth(0);
    m_stackWidget->insertWidget(0, m_searchNoteList);
    m_stackWidget->setCurrentWidget(m_searchNoteList);
    m_addTextBtn = new DPushButton(this);
    DFontSizeManager::instance()->bind(m_addTextBtn, DFontSizeManager::T5);
    m_addTextBtn->setText(tr("Click to Add TextNote"));
    m_addTextBtn->setFixedHeight(64);
}

void RightView::initConnection()
{
    connect(m_addTextBtn, &DPushButton::clicked, this, &RightView::onAddNote);
    connect(m_searchNoteList,SIGNAL(sigDelNote(VNoteItem *)), this, SLOT(onDelNote(VNoteItem *)));
    connect(m_searchNoteList, SIGNAL(sigUpdateNote(VNoteItem *)), this, SLOT(onUpdateNote(VNoteItem *)));
    connect(m_searchNoteList, SIGNAL(sigTextEditDetail(VNoteItem *, DTextEdit *,const QString &)), this, SIGNAL(sigTextEditDetail(VNoteItem *, DTextEdit *,const QString &)));
    connect(m_searchNoteList, SIGNAL(sigTextEditIsEmpty(VNoteItem *,bool)), this, SLOT(onTextEditIsEmpty(VNoteItem *,bool)));
}

void RightView::addNewNoteList(qint64 id)
{
//    RightNoteList *listWidget = new RightNoteList(m_stackWidget);
//    connect(listWidget,SIGNAL(sigDelNote(VNoteItem *)), this, SLOT(onDelNote(VNoteItem *)));
//    connect(listWidget, SIGNAL(sigUpdateNote(VNoteItem *)), this, SLOT(onUpdateNote(VNoteItem *)));
//    connect(listWidget, SIGNAL(sigTextEditDetail(VNoteItem *, DTextEdit *,const QString &)), this, SIGNAL(sigTextEditDetail(VNoteItem *, DTextEdit *,const QString &)));
//    connect(listWidget, SIGNAL(sigTextEditIsEmpty(VNoteItem *,bool)), this, SLOT(onTextEditIsEmpty(VNoteItem *,bool)));
//    QSharedPointer<VNOTE_ITEMS_MAP> listData(new VNOTE_ITEMS_MAP);
//    listData->notes = new VNOTE_ITEMS_DATA_MAP;
//    listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//    listWidget->setLineWidth(0);
//    listWidget->initNoteItem(id, listData.get());
//    m_stackWidget->addWidget(listWidget);
//    m_stackWidget->setCurrentWidget(listWidget);
//    m_data.insert(id, listData);
}

void RightView::noteDelByFolder(qint64 id)
{
    for (int i = 1; i < m_stackWidget->count(); i++) {
        RightNoteList *widget = static_cast<RightNoteList *>(m_stackWidget->widget(i));
        if (widget != nullptr && widget->getFolderId() == id) {
            m_stackWidget->removeWidget(widget);
            widget->deleteLater();
            adjustaddTextBtn();
        }
    }
}

void RightView::noteSwitchByFolder(qint64 id)
{
    if (m_searchKey.isEmpty()) {
        bool find = false;

        for (int i = 1; i < m_stackWidget->count(); i++) {
            RightNoteList *widget = static_cast<RightNoteList *>(m_stackWidget->widget(i));
            if (widget != nullptr && widget->getFolderId() == id) {
                m_stackWidget->setCurrentWidget(widget);
                find = true;
            }
        }
        if (find == false) {
            //addNewNoteList(id);没加载创建新的
        }
    }else { //搜索逻辑
//         auto it1 = m_data.find(id);
//         if(it1 != m_data.end())
//         {
//             VNOTE_ITEMS_MAP *data1 = it1->get();
//             if (data1 && data1->notes)
//             {
//                 m_searchNoteData.notes->clear();
//                 for (auto it2 : *data1->notes) {
//                     if (it2->noteText.contains(m_searchKey)) {
//                         m_searchNoteData.notes->insert(it2->noteId,it2);
//                     }
//                }
//                if(m_searchNoteData.notes->size())
//                {
//                    m_searchNoteList->initNoteItem(id,&m_searchNoteData, m_searchKey);
//                    m_stackWidget->setCurrentWidget(m_searchNoteList);
//                    emit sigSeachEditFocus();
//                }
//            }
//         }
    }
    adjustaddTextBtn();
}

void RightView::adjustaddTextBtn()
{
    RightNoteList *widget = static_cast<RightNoteList *>(m_stackWidget->currentWidget());
    int pos = 10;
    if (widget != nullptr) {
        pos += widget->getListHeight();
        int maxPos = this->height() - m_addTextBtn->height() - 70;
        if (pos > maxPos) {
            pos = maxPos;
        }
    }
    m_addTextBtn->setFixedWidth(this->width() - 18);
    m_addTextBtn->move(9, pos);
}

void RightView::onDelNote(VNoteItem *item)
{
    RightNoteList *widget = static_cast<RightNoteList *>(sender());
    widget->delNodeItem(item);
    if (widget == m_searchNoteList) {
        ;
    }
    auto it = m_data.find(item->folderId); //删除接口
//    if (it != m_data.end()) {
//        VNOTE_ITEMS_MAP *mapNotes = it.value().get();
//        if (mapNotes != nullptr && mapNotes->notes != nullptr) {
//            auto itNote = mapNotes->notes->find(item->noteId);
//            if (itNote != mapNotes->notes->end()) {
//                mapNotes->notes->erase(itNote);
//            }
//        }
//    }
    adjustaddTextBtn();
    m_addTextBtn->setEnabled(true);
}

void RightView::onAddNote() //添加文字记录
{
    RightNoteList *widget = static_cast<RightNoteList *>(m_stackWidget->currentWidget());
//    auto it = m_data.find(widget->getFolderId());
//    if (it != m_data.end()) {
//        VNOTE_ITEMS_MAP *mapNotes = it.value().get();
//        if (mapNotes != nullptr) {
//            VNOTE_ITEMS_DATA_MAP *noteData = mapNotes->notes;
//            if (noteData == nullptr) {
//                noteData = new VNOTE_ITEMS_DATA_MAP;
//            }
//            static int id = 0;
//            VNoteItem *tmp = new VNoteItem;
//            tmp->noteId = id++;
//            tmp->folderId = widget->getFolderId();
//            tmp->createTime = QDateTime::currentDateTime();
//            noteData->insert(id, tmp);
//            widget->addNodeItem(tmp);
//        }
//    }
    m_addTextBtn->setEnabled(false);
    adjustaddTextBtn();
}

void RightView::onUpdateNote(VNoteItem *item)
{
    qDebug() << "update:" << item->noteText;
}

void RightView::resizeEvent(QResizeEvent *event)
{
    adjustaddTextBtn();
    DWidget::resizeEvent(event);
}

QList<qint64> RightView::getNoteContainsKeyFolders(QString key) //内容包含查找内容
{
    QList<qint64> folderIds;
//    for (auto it1 : m_data) {
//        VNOTE_ITEMS_MAP *data1 = it1.get();
//        if (data1 && data1->notes) {
//            for (auto it2 : *data1->notes) {
//                if (it2->noteText.contains(key)) {
//                    folderIds.push_back(it2->folderId);
//                    break;
//                }
//            }
//        }
//    }
    return folderIds;
}

void RightView::onTextEditIsEmpty(VNoteItem *textNode,bool empty)
{
    Q_UNUSED(textNode);
    m_addTextBtn->setEnabled(!empty);
}

void RightView::setSearchKey(QString key)
{
    m_searchKey = key;
}
