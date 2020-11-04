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

#include <QStandardItemModel>
#include <QList>
#include <QDateTime>

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
struct VNoteFolder;

class FolderSelectView : public DTreeView
{
    Q_OBJECT
public:
    explicit FolderSelectView(QWidget *parent = nullptr);
    //移动
    void mouseMoveEvent(QMouseEvent *event) override;
    //单击
    void mousePressEvent(QMouseEvent *event) override;
private:
    QDateTime m_pressTime;
    QPoint m_pressPos;
};

class FolderSelectDialog : public DAbstractDialog
{
     Q_OBJECT
public:
    explicit FolderSelectDialog(QStandardItemModel *model, QWidget *parent = nullptr);
    void setNoteContext(const QString &text);
    void setFolderBlack(const QList<VNoteFolder *>& folders);
    void clearSelection();
    QModelIndex getSelectIndex();
    void onVNoteFolderSelectChange(const QItemSelection &selected, const QItemSelection &deselected);
protected:
    //初始化布局
    void initUI();
    //连接槽函数
    void initConnections();
private:
    DLabel      *m_noteInfo {nullptr};
    FolderSelectView   *m_view {nullptr};
    DWindowCloseButton *m_closeButton {nullptr};
    LeftViewSortFilter *m_model {nullptr};
    LeftViewDelegate *m_delegate {nullptr};
    DPushButton *m_cancelBtn {nullptr};
    DSuggestButton *m_confirmBtn {nullptr};
    DVerticalLine *m_buttonSpliter {nullptr};
};

#endif // FOLDERSELECTDIALOG_H
