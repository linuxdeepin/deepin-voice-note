#include "rightview.h"
#include "rightnotelist.h"

#include <QBoxLayout>
#include <QDebug>
#include <DStyledItemDelegate>

RightView::RightView(QWidget *parent)
    : QWidget(parent)
{
    initUi();
    initConnection();
}
void RightView::initUi()
{
    m_stackWidget = new  DStackedWidget (this);
    m_stackWidget->setContentsMargins(0,0,0,0);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(m_stackWidget);
    m_addTextBtn = new QPushButton(this);
    DFontSizeManager::instance()->bind(m_addTextBtn,DFontSizeManager::T5);
    m_addTextBtn->setText(tr("Click to Add TextNote"));
    m_addTextBtn->setFixedHeight(64);
    layout->addSpacing(64 + 68);
    this->setLayout(layout);
    m_addTextBtn->hide();
}
void RightView::initConnection()
{
    connect(m_addTextBtn, &QPushButton::clicked, this, &RightView::handleNoteAdd);
}
void RightView::handleFolderAdd(qint64 id)
{
    RightNoteList *data = new RightNoteList(id,m_stackWidget);
    data->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    data->setLineWidth(0);
    m_stackWidget->addWidget(data);
    m_stackWidget->setCurrentWidget(data);
    m_addTextBtn->show();
}
void RightView::handleFolderDel(qint64 id)
{
    for(int i = 0; i < m_stackWidget->count();i++)
    {
         RightNoteList *widget = static_cast<RightNoteList *>(m_stackWidget->widget(i));
         if(widget != nullptr && widget->getFolderId() == id)
         {
             m_stackWidget->removeWidget(widget);
             widget->deleteLater();
             adjustaddTextBtn();
         }
    }
    if(m_stackWidget->count() == 0)
    {
        m_addTextBtn->hide();
    }
}
void RightView::handleFolderSwitch(qint64 id)
{
     bool find = false;
     for(int i = 0; i < m_stackWidget->count();i++)
     {
         RightNoteList *widget = static_cast<RightNoteList *>(m_stackWidget->widget(i));
         if(widget != nullptr && widget->getFolderId() == id)
         {
             m_stackWidget->setCurrentWidget(widget);
             find = true;
         }
     }
     if(find == false)
     {
         handleFolderAdd(id);
     }
     adjustaddTextBtn();

}
void  RightView::adjustaddTextBtn()
{
    RightNoteList *widget = static_cast<RightNoteList *>(m_stackWidget->currentWidget());
    int pos = 10;
    if(widget != nullptr)
    {
        pos += widget->count() * 170;
        int maxPos = this->height() -  m_addTextBtn->height() - 68;
        if(pos > maxPos)
        {
            pos = maxPos;
        }
    }
    m_addTextBtn->setFixedWidth(this->width() - 20);
    m_addTextBtn->move(10,pos);
}
void RightView::handleNoteAdd()
{
     RightNoteList *widget = static_cast<RightNoteList *>(m_stackWidget->currentWidget());
     if(widget != nullptr)
     {
         widget->addTextNodeItem();
     }
     adjustaddTextBtn();
}
void RightView::resizeEvent(QResizeEvent * event)
{
    DWidget::resizeEvent(event);
    adjustaddTextBtn();
}
