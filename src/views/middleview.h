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

#ifndef MIDDLEVIEW_H
#define MIDDLEVIEW_H

#include <QDateTime>

#include <DListView>
#include <DMenu>
#include <DLabel>

DWIDGET_USE_NAMESPACE
class MiddleViewDelegate;
class MiddleViewSortFilter;
class MoveView;

struct VNoteItem;
//记事项列表
class MiddleView : public DListView
{
    Q_OBJECT
public:
    MiddleView(QWidget *parent = nullptr);
    //设置搜索关键字
    void setSearchKey(const QString &key);
    //设置记事本id
    void setCurrentId(qint64 id);
    //设置搜索无结果窗口是否可见
    void setVisibleEmptySearch(bool visible);
    //只有选中项有右键菜单
    void setOnlyCurItemMenuEnable(bool enable);
    //头部添加记事项
    void addRowAtHead(VNoteItem *note);
    //尾部追加记事项
    void appendRow(VNoteItem *note);
    //清除记事项
    void clearAll();
    //根据索引选中记事本
    void setCurrentIndex(int index);
    //记事项重命名
    void editNote();
    //导出文本
    void saveAsText();
    //到处语音
    void saveRecords();
    //关闭右键菜单
    void closeMenu();
    //获取记事项id
    qint64 getCurrentId();
    //获取记事项数目
    qint32 rowCount() const;
    //删除当前选中项
    VNoteItem *deleteCurrentRow();
    //获取当前选中项数据
    VNoteItem *getCurrVNotedata() const;
    //置顶/取消置顶
    void noteStickOnTop();
    //排序
    void sortView(bool adjustCurrentItemBar = true);

    QModelIndexList getAllSelectNote();
    void deleteModelIndexs(const QModelIndexList &indexs);
    //处理触摸屏slide事件
    void handleTouchSlideEvent(qint64 timeParam, double distY, QPoint point);
signals:
    void sigDragEnd();

public slots:
    //更新记事项
    void onNoteChanged();

protected:
    //鼠标事件
    //单击
    void mousePressEvent(QMouseEvent *event) override;
    //单击释放
    void mouseReleaseEvent(QMouseEvent *event) override;
    //双击
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    //移动
    void mouseMoveEvent(QMouseEvent *event) override;
    //事件过滤器
    bool eventFilter(QObject *o, QEvent *e) override;
    //按键事件
    void keyPressEvent(QKeyEvent *e) override;
    //关闭重命名编辑框触发
    void closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint) override;
    //触发拖动事件
    void startDrag(Qt::DropActions supportedActions) override;

private:
    //初始化代理模块
    void initDelegate();
    //初始化数据模块
    void initModel();
    //初始化右键菜单
    void initMenu();
    //初始化UI布局
    void initUI();
    //触发拖动操作
    void triggerDragNote();

    bool m_onlyCurItemMenuEnable {false};
    qint64 m_currentId {-1};
    QString m_searchKey;
    DLabel *m_emptySearch {nullptr};
    DMenu *m_noteMenu {nullptr};
    QStandardItemModel *m_pDataModel {nullptr};
    MiddleViewDelegate *m_pItemDelegate {nullptr};
    MiddleViewSortFilter *m_pSortViewFilter {nullptr};
    MoveView *m_MoveView {nullptr};

    //以下为实现触摸屏功能声明参数
    bool m_isTouchSliding {false};
    qint64 m_touchPressStartMs = 0;
    QPoint m_touchPressPoint;
};

#endif // MIDDLEVIEW_H
