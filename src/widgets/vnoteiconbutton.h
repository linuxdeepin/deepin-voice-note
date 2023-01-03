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

#ifndef VNOTEICONBUTTON_H
#define VNOTEICONBUTTON_H

#include <DApplicationHelper>
#include <DIconButton>

#include <QMouseEvent>

DWIDGET_USE_NAMESPACE

class VNoteIconButton : public DIconButton
{
    Q_OBJECT
public:
    explicit VNoteIconButton(QWidget *parent = nullptr, QString normal = "", QString hover = "", QString press = "");
    virtual ~VNoteIconButton() override;
    //是否区分主题
    void setSeparateThemIcons(bool separate);
    //设置按钮不可用的图标
    void SetDisableIcon(const QString &disableIcon);
    //设置按钮是否可用
    void setBtnDisabled(bool disabled);
    //获取焦点类型
    Qt::FocusReason getFocusReason();

protected:
    //重写鼠标事件
    //进入
    void enterEvent(QEvent *event) override;
    //离开
    void leaveEvent(QEvent *event) override;
    //单击
    void mousePressEvent(QMouseEvent *event) override;
    //释放
    void mouseReleaseEvent(QMouseEvent *event) override;
    //移动
    void mouseMoveEvent(QMouseEvent *event) override;
    //获取焦点
    void focusInEvent(QFocusEvent *e) override;

signals:

public slots:
    //主题切换
    void onThemeChanged(DGuiApplicationHelper::ColorType type);

private:
    //加载图片
    QPixmap loadPixmap(const QString &path);
    //更新图片
    void updateIcon();

    enum {
        Normal,
        Hover,
        Press,
        Disabled,
        MaxState
    };

    QString m_icons[MaxState];
    int m_state {Normal};

    //The Icon is different under different theme.
    bool m_separateThemeIcon {true};

    //Disable state
    bool m_isDisabled {false};

    Qt::FocusReason m_focusReason {Qt::NoFocusReason};
};

#endif // VNOTEICONBUTTON_H
