/*
* Copyright (C) 2019 ~ 2019 UnionTech Software Technology Co.,Ltd.
*
* Author:     liuyanga <liuyanga@uniontech.com>
*
* Maintainer: liuyanga <liuyanga@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "folderselectdialog.h"
#include "views/leftviewdelegate.h"
#include "views/leftviewsortfilter.h"

#include <QVBoxLayout>
#include <QDebug>
#include <QMouseEvent>
#include <QScrollBar>

#include <DApplication>
#include <DApplicationHelper>

FolderSelectView::FolderSelectView(QWidget *parent)
    :DTreeView (parent)
{

}

void FolderSelectView::mousePressEvent(QMouseEvent *event)
{
    if(event->source() == Qt::MouseEventSynthesizedByQt){
        m_pressPos = event->pos();
        m_pressTime = QDateTime::currentDateTime();
    }
    DTreeView::mousePressEvent(event);
}

void FolderSelectView::mouseMoveEvent(QMouseEvent *event)
{
    if (event->source() == Qt::MouseEventSynthesizedByQt) {
        QDateTime current = QDateTime::currentDateTime();
        qint64 timeNow = current.toMSecsSinceEpoch();
        qint64 timerDis = timeNow - m_pressTime.toMSecsSinceEpoch();
        if( timerDis < 250){
            double dist = event->pos().y() - m_pressPos.y();
            if (qAbs(dist) > 10) {
                //计算滑动速度
                double speed = ((qAbs(dist)) / timerDis) + 0.5;
                verticalScrollBar()->setSingleStep(static_cast<int>(20 * speed));
                if (dist > 0)
                    verticalScrollBar()->triggerAction(QScrollBar::SliderSingleStepSub);
                else {
                    verticalScrollBar()->triggerAction(QScrollBar::SliderSingleStepAdd);
                }
                m_pressTime = current;
                m_pressPos = event->pos();
            }
            return;
        }
    }
    DTreeView::mouseMoveEvent(event);
}

FolderSelectDialog::FolderSelectDialog(QStandardItemModel *model,QWidget *parent)
    :DAbstractDialog(parent)
{

    m_model = new LeftViewSortFilter(this);
    m_model->setSourceModel(model);
    initUI();
    initConnections();
    m_model->sort(0, Qt::DescendingOrder);

}

void FolderSelectDialog::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(10, 0, 10, 10);
    m_view = new FolderSelectView(this);
    DStyle::setFrameRadius(m_view, 20);
    m_view->setModel(m_model);
    m_view->setContextMenuPolicy(Qt::NoContextMenu);
    m_delegate = new LeftViewDelegate(m_view);
    m_delegate->setDrawNotesNum(false);
    m_view->setItemDelegate(m_delegate);
    m_view->setHeaderHidden(true);
    m_view->setItemsExpandable(false);
    m_view->setIndentation(0);
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->installEventFilter(this);
    QModelIndex notepadRootIndex = m_model->index(0,0);
    m_view->expand(notepadRootIndex);

    m_closeButton = new DWindowCloseButton(this);
    m_closeButton->setFocusPolicy(Qt::NoFocus);
    m_closeButton->setIconSize(QSize(50, 50));

    DLabel *labMove = new DLabel(this);
    DFontSizeManager::instance()->bind(labMove, DFontSizeManager::T6);
    labMove->setText("移动笔记");
    QHBoxLayout *titleLayout = new QHBoxLayout();
    titleLayout->setSpacing(0);
    titleLayout->setContentsMargins(10, 0, 0, 0);
    titleLayout->addStretch();
    titleLayout->addWidget(labMove,0, Qt::AlignCenter | Qt::AlignVCenter);
    titleLayout->addStretch();
    titleLayout->addWidget(m_closeButton, 0, Qt::AlignRight | Qt::AlignVCenter);

    m_noteInfo = new DLabel(this);
    m_noteInfo->setForegroundRole(DPalette::TextTips);
    DFontSizeManager::instance()->bind(m_noteInfo, DFontSizeManager::T6);
    QHBoxLayout *actionBarLayout = new QHBoxLayout();
    actionBarLayout->setSpacing(10);
    actionBarLayout->setContentsMargins(0, 0, 0, 0);

    m_cancelBtn = new DPushButton(this);
    m_confirmBtn = new DSuggestButton(this);

    m_cancelBtn->setText(DApplication::translate("LeftviewDialog", "Cancel"));
    m_confirmBtn->setText(DApplication::translate("LeftviewDialog", "Confirm"));

    m_buttonSpliter = new DVerticalLine(this);
    DPalette pa = DApplicationHelper::instance()->palette(m_buttonSpliter);
    QColor splitColor = pa.color(DPalette::ItemBackground);
    pa.setBrush(DPalette::Background, splitColor);
    m_buttonSpliter->setPalette(pa);
    m_buttonSpliter->setBackgroundRole(QPalette::Background);
    m_buttonSpliter->setAutoFillBackground(true);
    m_buttonSpliter->setFixedSize(4, 28);

    actionBarLayout->addWidget(m_cancelBtn);
    //actionBarLayout->addSpacing(8);
    actionBarLayout->addWidget(m_buttonSpliter);
    // actionBarLayout->addSpacing(8);
    actionBarLayout->addWidget(m_confirmBtn);

    mainLayout->addLayout(titleLayout);
    mainLayout->addWidget(m_noteInfo, 0, Qt::AlignCenter | Qt::AlignVCenter);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(m_view, 1);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(actionBarLayout);
    this->setLayout(mainLayout);
}

void FolderSelectDialog::initConnections()
{
    connect(m_cancelBtn, &DPushButton::clicked, this, [=]() {
        this->reject();
    });

    connect(m_confirmBtn, &DPushButton::clicked, this, [=]() {
        this->accept();
    });

    connect(m_closeButton, &DWindowCloseButton::clicked, this, [this]() {
        this->close();
    });
    connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &FolderSelectDialog::onVNoteFolderSelectChange);
}

void FolderSelectDialog::setNoteContext(const QString &text)
{
    m_noteInfo->setText(text);
}

QModelIndex FolderSelectDialog::getSelectIndex()
{
    QModelIndex index = m_view->currentIndex();
    QItemSelectionModel *model = m_view->selectionModel();
    if (!model->isSelected(index)) {
        index  = QModelIndex();
    }
    return  index;
}

void FolderSelectDialog::setFolderBlack(const QList<VNoteFolder* > &folders)
{
    m_model->setBlackFolders(folders);
}

void FolderSelectDialog::clearSelection()
{
    m_view->clearSelection();
    m_confirmBtn->setEnabled(false);
}

void FolderSelectDialog::onVNoteFolderSelectChange(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected);
    m_confirmBtn->setEnabled(!!selected.indexes().size());
}
