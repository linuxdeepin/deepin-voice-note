/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
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

#include "leftviewdelegate.h"
#include "common/vnoteforlder.h"
#include "common/standarditemcommon.h"

#include "db/vnotefolderoper.h"

#include <QPainter>
#include <QPainterPath>
#include <QLineEdit>

#include <DApplication>
#include <DApplicationHelper>
#include <DLog>

/**
 * @brief LeftViewDelegate::LeftViewDelegate
 * @param parent
 */
LeftViewDelegate::LeftViewDelegate(QAbstractItemView *parent)
    : DStyledItemDelegate(parent)
    , m_treeView(parent)
{
    init();
}

/**
 * @brief LeftViewDelegate::init
 */
void LeftViewDelegate::init()
{
    m_parentPb = DApplicationHelper::instance()->palette(m_treeView);
    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this,
            &LeftViewDelegate::handleChangeTheme);
}

/**
 * @brief LeftViewDelegate::createEditor
 * @param parent
 * @param option
 * @param index
 * @return 创建的编辑器
 */
QWidget *LeftViewDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                                        const QModelIndex &index) const
{
    Q_UNUSED(index)
    Q_UNUSED(option)
    QLineEdit *editBox = new QLineEdit(parent);
    editBox->setMaxLength(MAX_FOLDER_NAME_LEN);
    editBox->setFixedSize(132, 30);
    return editBox;
}

/**
 * @brief LeftViewDelegate::setEditorData
 * @param editor
 * @param index
 */
void LeftViewDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QVariant var = index.data(Qt::UserRole + 2);
    VNoteFolder *data = static_cast<VNoteFolder *>(var.value<void *>());
    QLineEdit *edit = static_cast<QLineEdit *>(editor);
    edit->setText(data->name);
}

/**
 * @brief LeftViewDelegate::setModelData
 * @param editor
 * @param model
 * @param index
 */
void LeftViewDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                    const QModelIndex &index) const
{
    Q_UNUSED(model);
    QLineEdit *edit = static_cast<QLineEdit *>(editor);
    QString text = edit->displayText();

    //Truncate the name if name length exceed 64 chars
    if (text.length() > MAX_FOLDER_NAME_LEN) {
        text = text.left(MAX_FOLDER_NAME_LEN);
    }

    if (!text.isEmpty()) {
        if (StandardItemCommon::getStandardItemType(index) == StandardItemCommon::NOTEPADITEM) {
            VNoteFolder *folderdata = static_cast<VNoteFolder *>(StandardItemCommon::getStandardItemData(index));
            if (folderdata && folderdata->name != text) {
                VNoteFolderOper folderOper(folderdata);
                folderOper.renameVNoteFolder(text);
            }
        }
    }
}

/**
 * @brief LeftViewDelegate::updateEditorGeometry
 * @param editor
 * @param option
 * @param index
 */
void LeftViewDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                                            const QModelIndex &index) const
{
    Q_UNUSED(index)
    QLineEdit *edit = static_cast<QLineEdit *>(editor);
    edit->move(option.rect.x() + 50, option.rect.y() + 8);
}

/**
 * @brief LeftViewDelegate::handleChangeTheme
 */
void LeftViewDelegate::handleChangeTheme()
{
    m_parentPb = DApplicationHelper::instance()->palette(m_treeView);
    m_treeView->update(m_treeView->currentIndex());
}

/**
 * @brief LeftViewDelegate::sizeHint
 * @param option
 * @param index
 * @return 列表项大小
 */
QSize LeftViewDelegate::sizeHint(const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    StandardItemCommon::StandardItemType type = StandardItemCommon::getStandardItemType(index);
    switch (type) {
    case StandardItemCommon::NOTEPADROOT:
        return QSize(option.rect.width(), 1); //隐藏记事本一级目录
    case StandardItemCommon::NOTEPADITEM:
        return QSize(option.rect.width(), 47);
    default:
        return DStyledItemDelegate::sizeHint(option, index);
    }
}

/**
 * @brief LeftViewDelegate::paint
 * @param painter
 * @param option
 * @param index
 */
void LeftViewDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    StandardItemCommon::StandardItemType type = StandardItemCommon::getStandardItemType(index);
    switch (type) {
    case StandardItemCommon::NOTEPADROOT:
        return paintNoteRoot(painter, option, index);
    case StandardItemCommon::NOTEPADITEM:
        return paintNoteItem(painter, option, index);
    default:
        return DStyledItemDelegate::paint(painter, option, index);
    }
}

/**
 * @brief LeftViewDelegate::paintNoteRoot
 * @param painter
 * @param option
 * @param index
 */
void LeftViewDelegate::paintNoteRoot(QPainter *painter, const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const
{
    Q_UNUSED(painter)
    Q_UNUSED(index)
    Q_UNUSED(option)
}

/**
 * @brief LeftViewDelegate::paintNoteItem
 * @param painter
 * @param option
 * @param index
 */
void LeftViewDelegate::paintNoteItem(QPainter *painter, const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const
{
    if (!index.isValid()) {
        return;
    }
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHints(QPainter::SmoothPixmapTransform);
    QRect paintRect(option.rect.x() + 10, option.rect.y() + 5,
                    option.rect.width() - 20, option.rect.height() - 10);
    QPainterPath path;
    const int radius = 8;
    path.moveTo(paintRect.bottomRight() - QPoint(0, radius));
    path.lineTo(paintRect.topRight() + QPoint(0, radius));
    path.arcTo(QRect(QPoint(paintRect.topRight() - QPoint(radius * 2, 0)), QSize(radius * 2, radius * 2)), 0, 90);
    path.lineTo(paintRect.topLeft() + QPoint(radius, 0));
    path.arcTo(QRect(QPoint(paintRect.topLeft()), QSize(radius * 2, radius * 2)), 90, 90);
    path.lineTo(paintRect.bottomLeft() - QPoint(0, radius));
    path.arcTo(QRect(QPoint(paintRect.bottomLeft() - QPoint(0, radius * 2)), QSize(radius * 2, radius * 2)), 180, 90);
    path.lineTo(paintRect.bottomLeft() + QPoint(radius, 0));
    path.arcTo(QRect(QPoint(paintRect.bottomRight() - QPoint(radius * 2, radius * 2)), QSize(radius * 2, radius * 2)), 270, 90);

    bool enable = true;
    if (option.state & QStyle::State_Selected) {
        QColor fillColor = option.palette.color(DPalette::Normal, DPalette::Highlight);
        painter->setBrush(QBrush(fillColor));
        painter->fillPath(path, painter->brush());
        painter->setPen(QPen(Qt::white));
    } else {
        if (m_enableItem == false || !(option.state & QStyle::State_Enabled)) {
            painter->setBrush(QBrush(m_parentPb.color(DPalette::Disabled, DPalette::ItemBackground)));
            painter->fillPath(path, painter->brush());
            painter->setPen(QPen(m_parentPb.color(DPalette::Disabled, DPalette::TextTitle)));
            enable = false;
        } else {
            if (option.state & QStyle::State_MouseOver) {
                painter->setBrush(QBrush(m_parentPb.color(DPalette::Light)));
                painter->fillPath(path, painter->brush());
                painter->setPen(QPen(m_parentPb.color(DPalette::Normal, DPalette::TextTitle)));
            } else {
                painter->setBrush(QBrush(m_parentPb.color(DPalette::Normal, DPalette::ItemBackground)));
                painter->fillPath(path, painter->brush());
                painter->setPen(QPen(m_parentPb.color(DPalette::Normal, DPalette::TextTitle)));
            }
        }
    }
    VNoteFolder *data = static_cast<VNoteFolder *>(StandardItemCommon::getStandardItemData(index));
    if (data != nullptr) {
        VNoteFolderOper folderOps(data);

        QFontMetrics fontMetrics = painter->fontMetrics();
        QString strNum = QString::number(folderOps.getNotesCount());
        int numWidth = fontMetrics.width(strNum);
        int iconSpace = (paintRect.height() - 24) / 2;
        QRect iconRect(paintRect.left() + 11, paintRect.top() + iconSpace, 24, 24);
        QRect numRect(paintRect.right() - numWidth - 7, paintRect.top(), numWidth, paintRect.height());
        QRect nameRect(iconRect.right() + 12, paintRect.top(),
                       numRect.left() - iconRect.right() - 15, paintRect.height());

        if(m_drawNotesNum){
            painter->drawText(numRect, Qt::AlignRight | Qt::AlignVCenter, strNum);
        }


        if (enable == false) {
            painter->drawPixmap(iconRect, data->UI.grayIcon);
        } else {
            painter->drawPixmap(iconRect, data->UI.icon);
        }

        QString elideText = fontMetrics.elidedText(data->name, Qt::ElideRight, nameRect.width());
        painter->drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, elideText);
    }
    painter->restore();
}

/**
 * @brief LeftViewDelegate::setEnableItem
 * @param enable true 列表项可用
 */
void LeftViewDelegate::setEnableItem(bool enable)
{
    m_enableItem = enable;
}

void LeftViewDelegate::setDrawNotesNum(bool enable)
{
    m_drawNotesNum = enable;
}

