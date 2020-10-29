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
    void mouseMoveEvent(QMouseEvent *eve)override;
    void mousePressEvent(QMouseEvent *eve)override;
    void mouseReleaseEvent(QMouseEvent *eve)override;
private:
    int m_pressPointY = 0;
    qint64 pressStartMs = 0;
    bool m_mouseMoved{false};
};
#endif // LEFTVIEWLIST_H
