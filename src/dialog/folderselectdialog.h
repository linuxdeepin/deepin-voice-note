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
#ifndef FOLDERSELECTDIALOG_H
#define FOLDERSELECTDIALOG_H

#include <DPushButton>
#include <DWarningButton>
#include <DVerticalLine>
#include <DTreeView>
#include <DStyledItemDelegate>
#include <DWindowCloseButton>
#include <DSuggestButton>
#include <DLabel>
#include <DAbstractDialog>

DWIDGET_USE_NAMESPACE

class LeftViewDelegate;
class LeftViewSortFilter;

class FolderSelectDialog : public DAbstractDialog
{
     Q_OBJECT
public:
    explicit FolderSelectDialog(LeftViewSortFilter *model, QWidget *parent = nullptr);
    void setNoteContext(const QString &text);
    QModelIndex getSelectIndex();
protected:
    //初始化布局
    void initUI();
    //连接槽函数
    void initConnections();
private:
    DLabel      *m_noteInfo {nullptr};
    DTreeView   *m_view {nullptr};
    DWindowCloseButton *m_closeButton {nullptr};
    LeftViewSortFilter *m_model {nullptr};
    LeftViewDelegate *m_delegate {nullptr};
    DPushButton *m_cancelBtn {nullptr};
    DSuggestButton *m_confirmBtn {nullptr};
    DVerticalLine *m_buttonSpliter {nullptr};

};

#endif // FOLDERSELECTDIALOG_H
