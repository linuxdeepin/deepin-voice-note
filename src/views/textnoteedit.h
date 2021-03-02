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

#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include <DTextEdit>
DWIDGET_USE_NAMESPACE
struct VNoteItem;
struct VNoteBlock;

class TextNoteEditPrivate;

class TextNoteEdit : public DTextEdit
{
    Q_OBJECT
public:
    explicit TextNoteEdit(QWidget *parent = nullptr);
    //选中
    void selectText(const QPoint &globalPos, QTextCursor::MoveOperation op);
    //清除选中
    void clearSelection();
    //删除选中内容
    void removeSelectText();
    //获取选中内容
    QString getSelectFragment();
    //是否有选中
    bool hasSelection();
    bool isSelectAll();
    bool hasSelectVoice();
    void setSelectVoice(bool flag);
    void setSelectAll(bool flag);
signals:
    //获取焦点
    void sigFocusIn(Qt::FocusReason reson);
    //失去焦点
    void sigFocusOut();

private:
    bool m_isSelectAll {false};
    bool m_hasSelectVoice {false};
    bool m_menuPop {false};
    //替换制表符'\t'
    void indentText();

protected:
    //获取焦点
    void focusInEvent(QFocusEvent *e) override;
    //失去焦点
    void focusOutEvent(QFocusEvent *e) override;
    //鼠标滚轮
    void wheelEvent(QWheelEvent *e) override;
    //编辑器自带右键菜单弹出
    void contextMenuEvent(QContextMenuEvent *e) override;
    //按键处理
    void keyPressEvent(QKeyEvent *e) override;
    //鼠标事件
    //单击
    void mousePressEvent(QMouseEvent *event) override;
    //单击释放
    void mouseReleaseEvent(QMouseEvent *event) override;
    //滚动
    void mouseMoveEvent(QMouseEvent *event) override;
    //双击
    void mouseDoubleClickEvent(QMouseEvent *event) override;
};

#endif // TEXTEDITITEM_H
