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

#include "vnoterightmenu.h"
#include <DTreeView>

#include <QStandardItemModel>

DWIDGET_USE_NAMESPACE

class LeftViewDelegate;
class LeftViewSortFilter;
class LeftviewDialog;

struct VNoteFolder;
struct VNoteItem;

//记事本列表
class LeftView : public DTreeView
{
    Q_OBJECT
public:
    //触摸屏事件
    enum TouchState {
        TouchNormal,
        TouchMoving,
        TouchDraging,
        TouchPressing
    };

    explicit LeftView(QWidget *parent = nullptr);
    //获取记事本项父节点
    QStandardItem *getNotepadRoot();
    //获取记事本项父节点索引
    QModelIndex getNotepadRootIndex();
    //选中默认记事本项
    QModelIndex setDefaultNotepadItem();
    //清除选中后，恢复选中
    QModelIndex restoreNotepadItem();
    //只有选中项有右键菜单
    void setOnlyCurItemMenuEnable(bool enable);
    //添加记事本
    void addFolder(VNoteFolder *folder);
    //从列表后追加记事本
    void appendFolder(VNoteFolder *folder);
    //记事本项重命名
    void editFolder();
    //记事本排序
    void sort();
    //关闭右键菜单
    void closeMenu();
    //获取记事本个数
    int folderCount();
    //删除选中的记事本
    VNoteFolder *removeFolder();
    //获取当前顺序所有记事本编号
    QString getFolderSortId();
    //给当前记事本列表绑定排序编号
    bool setFolderSortNum();

    //笔记移动
    bool popupMoveDlg();
    //笔记移动
    bool doNoteMove(const QModelIndexList &src, const QModelIndex &dst);
    //更新当前记事本排序编号
    void updateFolderSortNum(const QModelIndex &sorceIndex, const QModelIndex &targetIndex);
    //更新移动笔记列表
    void setMoveList(const QModelIndexList &moveList);
    //更新触摸屏一指状态
    void setTouchState(const TouchState &touchState);

signals:

protected:
    //单击
    void mousePressEvent(QMouseEvent *event) override;
    //单击释放
    void mouseReleaseEvent(QMouseEvent *event) override;
    //双击
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    //移动
    void mouseMoveEvent(QMouseEvent *event) override;
    //按键事件
    void keyPressEvent(QKeyEvent *e) override;
    //关闭重命名编辑框触发
    void closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint) override;

    //放下动作
//    void dropEvent(QDropEvent *event) override;
    //托到进入窗口动作
    void dragEnterEvent(QDragEnterEvent *event) override;
    //拖着物体在窗口移动
    void dragMoveEvent(QDragMoveEvent *event) override;
    //拖走了没有释放
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    //设置当前点击index选中
    void selectCurrentOnTouch();
    //延时更新代理中拖拽状态
    void updateDragingState();
    //处理触摸屏drag事件
    void handleDragEvent();
    //处理触摸屏鼠标move事件，区分点击、滑动、拖拽、长按功能
    void doTouchMoveEvent(QMouseEvent *eve);
    //处理触摸屏slide事件
    void handleTouchSlideEvent(qint64 timeParam, double distY, QPoint point);
private:
    //初始化代理模块
    void initDelegate();
    //初始化数据管理模块
    void initModel();
    //初始化记事本项父节点
    void initNotepadRoot();
    //初始化右键菜单
    void initMenu();

    VNoteRightMenu *m_notepadMenu {nullptr};
    QStandardItemModel *m_pDataModel {nullptr};
    LeftViewDelegate *m_pItemDelegate {nullptr};
    LeftViewSortFilter *m_pSortViewFilter {nullptr};
    bool m_onlyCurItemMenuEnable {false};
    QModelIndexList m_moveList;
    //以下为实现拖拽功能声明参数
    QModelIndex m_index;
    LeftviewDialog *m_folderSelectDlg {nullptr};
    bool m_isDraging {false};

    //以下为实现触摸屏功能声明参数
    qint64 m_touchInterval = 0;
    qint64 m_touchPressStartMs = 0;
    QPoint m_touchPressPoint;
    TouchState m_touchState {TouchNormal};
};

#endif // LEFTVIEW_H
