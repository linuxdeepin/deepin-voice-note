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

#ifndef LEFTVIEW_H
#define LEFTVIEW_H

#include <QStandardItemModel>

#include <DTreeView>
#include <DMenu>
DWIDGET_USE_NAMESPACE

class LeftViewDelegate;
class LeftViewSortFilter;

struct VNoteFolder;

class LeftView : public DTreeView
{
    Q_OBJECT
public:
    explicit LeftView(QWidget *parent = nullptr);
    QStandardItem *getNotepadRoot();

    QModelIndex getNotepadRootIndex();
    QModelIndex setDefaultNotepadItem();
    QModelIndex restoreNotepadItem();

    void setOnlyCurItemMenuEnable(bool enable);
    void addFolder(VNoteFolder *folder);
    void appendFolder(VNoteFolder *folder);
    void editFolder();
    void sort();
    void closeMenu();
    int folderCount();

    VNoteFolder *removeFolder();

signals:

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *e) override;
    void closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint) override;

private:
    void initDelegate();
    void initModel();
    void initNotepadRoot();
    void initMenu();
    DMenu *m_notepadMenu {nullptr};
    QStandardItemModel *m_pDataModel {nullptr};
    LeftViewDelegate *m_pItemDelegate {nullptr};
    LeftViewSortFilter *m_pSortViewFilter {nullptr};
    bool m_onlyCurItemMenuEnable {false};
};

#endif // LEFTVIEW_H
