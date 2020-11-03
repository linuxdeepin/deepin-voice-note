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
#include "leftviewdialog.h"
#include "views/leftviewdelegate.h"
#include "views/leftviewsortfilter.h"
#include "common/vnoteitem.h"

#include <QVBoxLayout>
#include <QKeyEvent>

#include <DApplicationHelper>
#include <DLabel>
#include <DApplication>
#include <DStyle>
#include <DLog>

LeftviewDialog::LeftviewDialog(LeftViewSortFilter *model, QWidget *parent)
    : DAbstractDialog(parent)
    , m_model(model)
{
    initUI();
    initConnections();
}

void LeftviewDialog::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(10, 0, 10, 10);
    m_view = new leftviewlist(this);
    DStyle::setFrameRadius(m_view, 20);
    m_view->setModel(m_model);
    m_delegate = new LeftViewDelegate(m_view);
    m_delegate->setDrawNotesNum(false);
    m_view->setItemDelegate(m_delegate);
    m_view->setHeaderHidden(true);
    m_view->setItemsExpandable(false);
    m_view->setIndentation(0);
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->installEventFilter(this);
    QModelIndex notepadRootIndex = m_model->index(0, 0);
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
    titleLayout->addWidget(labMove, 0, Qt::AlignCenter | Qt::AlignVCenter);
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

void LeftviewDialog::initConnections()
{
    connect(m_cancelBtn, &DPushButton::clicked, this, [ = ]() {
        this->reject();
    });

    connect(m_confirmBtn, &DPushButton::clicked, this, [ = ]() {
        this->accept();
    });

    connect(m_closeButton, &DWindowCloseButton::clicked, this, [this]() {
        this->close();
    });
}

bool LeftviewDialog::eventFilter(QObject *o, QEvent *e)
{
    Q_UNUSED(o);
    if (e->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = reinterpret_cast<QKeyEvent *>(e);
        if (keyEvent->key() == Qt::Key_PageUp || keyEvent->key() == Qt::Key_PageDown) {
            return true;
        } else {
            if (0 == m_view->currentIndex().row() && keyEvent->key() == Qt::Key_Up) {
                return true;
            }
        }
    }
    return false;
}

void LeftviewDialog::setNoteContext(const QString &text)
{
    m_noteInfo->setText(text);
}

void LeftviewDialog::clearSelection()
{
    m_view->clearSelection();
}

QModelIndex LeftviewDialog::getSelectIndex()
{
    QModelIndex index = m_view->currentIndex();
    QItemSelectionModel *model = m_view->selectionModel();
    if (!model->isSelected(index)) {
        index  = QModelIndex();
    }
    return  index;
}
