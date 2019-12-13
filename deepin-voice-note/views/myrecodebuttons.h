#ifndef MYRECODEBUTTONS_H
#define MYRECODEBUTTONS_H

#include <QWidget>
#include <DPushButton>

DWIDGET_USE_NAMESPACE
enum BUTTON_STATE {NORMAL=0, HOVER=1, PRESS=2, DISABLE=3, FOUCS=4};

class MyRecodeButtons : public DPushButton
{
    Q_OBJECT
public:
    explicit MyRecodeButtons(QString normal, QString press, QString hover, QString disable, QString foucs, QSize size, QWidget *parent = nullptr);
    ~MyRecodeButtons();
    void setPicChange(QString normal, QString press, QString hover, QString disable, QString foucs);
    void DisableBtn();
    void EnAbleBtn();
    void setBtnToNormal();

signals:
    void sigHoverd();
    //void sigLeave();

public slots:

protected:
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void enterEvent(QEvent *event);
    void leaveEvent(QEvent *event);

private:
    bool m_isPressed;
    bool m_isIn;
    bool m_isDisabled;
    //BUTTON_STATE m_state;
    QPixmap m_normal;
    QPixmap m_hover;
    QPixmap m_press;
    QPixmap m_disable;
    QPixmap m_foucs;
};

#endif // MYRECODEBUTTONS_H
