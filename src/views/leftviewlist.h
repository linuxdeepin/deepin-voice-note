#ifndef LEFTVIEWLIST_H
#define LEFTVIEWLIST_H

#include <QWidget>
#include <DTreeView>
#include <QMouseEvent>
#include <QDateTime>
#include <QScrollBar>
#include <QDebug>
DWIDGET_USE_NAMESPACE
//记事本列表
class leftviewlist : public DTreeView
{
    Q_OBJECT
public:
    explicit leftviewlist(QWidget *parent = nullptr);
    leftviewlist();
protected:
    //处理鼠标Move事件
    void mouseMoveEvent(QMouseEvent *eve)override;
    //处理鼠标Press事件
    void mousePressEvent(QMouseEvent *eve)override;
    //处理鼠标Release事件
    void mouseReleaseEvent(QMouseEvent *eve)override;
private:
    int m_pressPointY = 0;
    qint64 pressStartMs = 0;
    bool m_mouseMoved{false};
};
#endif // LEFTVIEWLIST_H
