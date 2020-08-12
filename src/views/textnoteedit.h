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
    void selectText(const QPoint &globalPos, QTextCursor::MoveOperation op);
    void clearSelection();
    void removeSelectText();
    QString getSelectFragment();
    bool hasSelection();

signals:
    void sigFocusIn();
    void sigFocusOut();

private:
    bool m_menuPop {false};
    void indentText();

protected:
    void focusInEvent(QFocusEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
};

#endif // TEXTEDITITEM_H
