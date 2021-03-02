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

#ifndef TEXTNOTEITEM_H
#define TEXTNOTEITEM_H

#include "common/vnoteitem.h"

class TextNoteEdit;

class TextNoteItem : public DetailItemWidget
{
    Q_OBJECT
public:
    explicit TextNoteItem(VNoteBlock *noteBlock, QWidget *parent = nullptr);
    //重写基类函数
    VNoteBlock *getNoteBlock();
    QTextCursor getTextCursor();
    void setTextCursor(const QTextCursor &cursor);
    void updateSearchKey(QString searchKey);
    bool textIsEmpty();
    QRect getCursorRect();
    void selectText(const QPoint &globalPos, QTextCursor::MoveOperation op);
    void selectText(QTextCursor::MoveOperation op);
    void removeSelectText();
    void selectAllText();
    void clearSelection();
    bool hasSelection();
    QTextDocumentFragment getSelectFragment();
    QTextDocument *getTextDocument();

    void pasteText();
    void setFocus(bool hasVoice);
    bool hasFocus();
    bool isSelectAll();
    //最后光标y坐标
    void setLastCursorHeight(int height);
    void setSelectVoice(bool flag);

signals:
    //光标位置改变
    void sigCursorHeightChange(QWidget *widget, int height);
    //内容改变
    void sigTextChanged();
    //获取焦点
    void sigFocusIn(Qt::FocusReason);
    //失去焦点
    void sigFocusOut();
    //选中改变
    void sigSelectionChanged();
public slots:
    //主题改变
    void onChangeTheme();
    //内容改变
    void onTextChange();
    //光标位置改变
    void onTextCursorChange();

private:
    //初始化布局
    void initUi();
    //连接槽函数
    void initConnection();
    bool m_isSearching {false};
    bool m_textDocumentUndo {false};
    int m_searchCount {0};
    int m_lastCursorHeight {0};
    VNoteBlock *m_noteBlock {nullptr};
    TextNoteEdit *m_textEdit {nullptr};
    QString m_serchKey;
};

#endif // TEXTNOTEITEM_H
