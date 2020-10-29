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
#ifndef LEFTVIEWDIALOG_H
#define LEFTVIEWDIALOG_H
#include "vnotebasedialog.h"
#include "leftviewlist.h"

#include <QVector>

#include <DPushButton>
#include <DWarningButton>
#include <DVerticalLine>
#include <DTreeView>
#include <DStyledItemDelegate>
#include <DWindowCloseButton>
#include <DSuggestButton>
#include <DLabel>

DWIDGET_USE_NAMESPACE

class LeftViewDelegate;
class LeftViewSortFilter;
struct VNoteItem;

class LeftviewDialog : public DAbstractDialog
{
public:
    explicit LeftviewDialog(LeftViewSortFilter *model, QWidget *parent = nullptr);
    void setNoteContext(const QString &text);
    QModelIndex getSelectIndex();
protected:
    //初始化布局
    void initUI();
    //连接槽函数
    void initConnections();
    //事件过滤器
    bool eventFilter(QObject *o, QEvent *e) override;
private:
    DLabel      *m_noteInfo {nullptr};
    leftviewlist   *m_view {nullptr};
    DWindowCloseButton *m_closeButton {nullptr};
    LeftViewSortFilter *m_model {nullptr};
    LeftViewDelegate *m_delegate {nullptr};
    DPushButton *m_cancelBtn {nullptr};
    DSuggestButton *m_confirmBtn {nullptr};
    DVerticalLine *m_buttonSpliter {nullptr};
};

#endif // LEFTVIEWDIALOG_H
