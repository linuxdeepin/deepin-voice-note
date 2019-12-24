#include "myrecodebuttons.h"
#include "common/utils.h"
#include <QPainter>
#include <QDebug>

MyRecodeButtons::MyRecodeButtons(QString normal, QString press, QString hover, QString disable, QString foucs, QSize size, QWidget *parent) : DPushButton(parent)
{
    this->setFixedSize(size);
    setPicChange(normal, press, hover,  disable, foucs);
    m_isPressed = false;
    m_isIn = false;
    m_isDisabled = false;
}

MyRecodeButtons::~MyRecodeButtons()
{

}

void MyRecodeButtons::setPicChange(QString normal, QString press, QString hover, QString disable, QString foucs)
{
    //Intancer::get_Intancer()->getApp()
    m_normal = Utils::renderSVG(normal, QSize(this->width(), this->height()), qApp);
    m_press = Utils::renderSVG(press, QSize(this->width(), this->height()), qApp);
    m_hover = Utils::renderSVG(hover, QSize(this->width(), this->height()), qApp);
    m_disable = Utils::renderSVG(disable, QSize(this->width(), this->height()), qApp);
    m_foucs = Utils::renderSVG(foucs, QSize(this->width(), this->height()), qApp);
}

void MyRecodeButtons::DisableBtn()
{
    this->setDisabled(true);
    m_isDisabled = true;
}

void MyRecodeButtons::EnAbleBtn()
{
    this->setDisabled(false);
    m_isDisabled = false;
}

void MyRecodeButtons::setBtnToNormal()
{
    m_isIn = false;
    m_isDisabled = false;
}

void MyRecodeButtons::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    painter.setRenderHints(QPainter::HighQualityAntialiasing |
                           QPainter::SmoothPixmapTransform |
                           QPainter::Antialiasing);
    if (m_isDisabled) {
        //disable
        painter.drawPixmap(rect(), m_disable);
    } else if (!m_isIn && !m_isDisabled) {
        //Normal
        painter.drawPixmap(rect(), m_normal);
    } else if (!m_isPressed && m_isIn) {
        //hover
        painter.drawPixmap(rect(), m_hover);
    } else if (m_isPressed && m_isIn) {
        //press
        painter.drawPixmap(rect(), m_press);
    }

}

void MyRecodeButtons::mousePressEvent(QMouseEvent *event)
{
    DPushButton::mousePressEvent(event);
    m_isPressed = true;
    repaint();
}

void MyRecodeButtons::mouseReleaseEvent(QMouseEvent *event)
{
    m_isPressed = false;
    DPushButton::mouseReleaseEvent(event);
    repaint();
}

void MyRecodeButtons::enterEvent(QEvent *event)
{
    DPushButton::enterEvent(event);
    m_isIn = true;
    if (m_isDisabled) {
        emit sigHoverd();
    }
}

void MyRecodeButtons::leaveEvent(QEvent *event)
{
    DPushButton::leaveEvent(event);
    m_isIn = false;
    m_isPressed = false;
    //emit sigLeave();
}

void MyRecodeButtons::mouseMoveEvent(QMouseEvent *event)
{
    DPushButton::mouseMoveEvent(event);
}
