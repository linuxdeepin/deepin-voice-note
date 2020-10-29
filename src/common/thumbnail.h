/*
* Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
*
* Author:     zhangteng <zhangteng@uniontech.com>
* Maintainer: zhangteng <zhangteng@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef THUMBNAIL_H
#define THUMBNAIL_H

#include <QAbstractItemView>
#include <QHBoxLayout>

#include <DPushButton>
#include <DLabel>
#include <DApplicationHelper>
#include <DWidget>

#define RADIUS 17             //窗口边角的弧度
#define ELLIPSE_RADIUS 8     //内部小圆半径
#define RECT 10               //图标长/宽的一半
#define TEXT_LENGTH 115       //文字长度

DWIDGET_USE_NAMESPACE

class thumbnail : public DWidget
{
    Q_OBJECT
public:
    thumbnail(QAbstractItemView *parent = nullptr);

    //记事本缩略图
    void setupthumbnail(QIcon icon, const QString &str, const QString &text1);
    //笔记缩略图
    void setupthumbnail(const QString &text);

private:
    void initUi();
    void paintEvent(QPaintEvent *);

private:
    DPushButton *m_imageBtn = nullptr;
    DLabel *m_textLbl = nullptr;
    DLabel *m_textLbl1 = nullptr;
    QHBoxLayout *m_layout;
    DPalette m_parentPb;
    QAbstractItemView *m_itemView {nullptr};
};

#endif // THUMBNAIL_H
