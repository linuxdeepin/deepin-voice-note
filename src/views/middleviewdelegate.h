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

#include <QPixmap>

DWIDGET_USE_NAMESPACE

struct VNoteItem;

class MiddleViewDelegate : public DStyledItemDelegate
{
    Q_OBJECT
public:
    explicit MiddleViewDelegate(QAbstractItemView *parent = nullptr);
    //主题改变处理
    void handleChangeTheme();
    //设置搜索关键字
    void setSearchKey(const QString &key);
    //设置列表项是否可用
    void setEnableItem(bool enable);
    //设置编辑状态
    void setEditIsVisible(bool isVisible);
    //设置tab焦点标志
    void setTabFocus(bool focus);
    //判断是否tab焦点状态
    bool isTabFocus();

    const int MAX_TITLE_LEN = 64;

protected:
    //绘制列表项
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
    //返回列表项大小
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
    //创建编辑器
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    //编辑框设置初始内容
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    //编辑完成
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    //调整编辑框位置
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                              const QModelIndex &index) const override;

private:
    //绘制列表项背景
    void paintItemBase(QPainter *painter, const QStyleOptionViewItem &option,
                       const QRect &rect, bool &isSelect) const;
    //绘制常规项
    void paintNormalItem(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const;
    //绘制搜索项
    void paintSearchItem(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const;

    void setPaintPath(const QRect &bgRect, QPainterPath &path, const int xDifference, const int yDifference, const int radius) const;

    void paintTabFocusBackground(QPainter *painter, const QStyleOptionViewItem &option, const QRect &bgRect) const;

    QAbstractItemView *m_parentView {nullptr};
    QString m_searchKey;
    DPalette m_parentPb;
    bool m_enableItem {true};
    bool m_editVisible {false};
    bool m_tabFocus {false};
    QPixmap m_topIcon;
};

#endif // LEFTVIEWDELEGATE_H
