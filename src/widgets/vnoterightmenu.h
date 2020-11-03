#pragma once

#include <DMenu>

#include <QMouseEvent>
#include <QDebug>
#include <QTimer>

DWIDGET_USE_NAMESPACE
class VNoteRightMenu : public DMenu
{
    Q_OBJECT
public:
    explicit VNoteRightMenu(QWidget *parent = nullptr);
    ~VNoteRightMenu() override;
    void setPressPointY(int value);

protected:
    void initConnections();
    void mouseMoveEvent(QMouseEvent *eve)override;
    void mouseReleaseEvent(QMouseEvent *eve)override;
    void closeEvent(QCloseEvent *eve)override;
private:
    int m_pressPointY = 0;
    bool m_moved {false};
    QTimer *m_timer;
signals:
    void menuTouchMoved();
    void menuTouchReleased();
private slots:

};
