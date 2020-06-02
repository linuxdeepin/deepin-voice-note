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

#ifndef LEFTVIEWDELEGATE_H
#define LEFTVIEWDELEGATE_H

#include <DStyledItemDelegate>
DWIDGET_USE_NAMESPACE

class MiddleViewDelegate : public DStyledItemDelegate
{
    Q_OBJECT
public:
    MiddleViewDelegate(QAbstractItemView *parent = nullptr);
    void handleChangeTheme();
    void setSearchKey(const QString &key);
    void setEnableItem(bool enable);
    void setEditIsVisible(bool isVisible);
    const int MAX_TITLE_LEN = 64;
protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,  const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                              const QModelIndex &index) const override;
private:
    void paintItemBase(QPainter *painter, const QStyleOptionViewItem &option,
                       const QRect &rect, bool &isSelect) const;
    void paintNormalItem(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const;
    void paintSearchItem(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const;

    QAbstractItemView *m_parentView {nullptr};
    QString m_searchKey;
    DPalette m_parentPb;
    bool     m_enableItem {true};
    bool     m_editVisible {false};
};

#endif // LEFTVIEWDELEGATE_H
