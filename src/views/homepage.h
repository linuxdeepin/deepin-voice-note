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

#ifndef INITEMPTYPAGE_H
#define INITEMPTYPAGE_H

#include "widgets/vnoteiconbutton.h"

#include <DLabel>
#include <DPushButton>
#include <DSuggestButton>

#include <QWidget>

DWIDGET_USE_NAMESPACE
//无数据主页
class HomePage : public QWidget
{
    Q_OBJECT
public:
    explicit HomePage(QWidget *parent = nullptr);

signals:
    //添加记事本信号
    void sigAddFolderByInitPage();

protected:
    bool eventFilter(QObject *o, QEvent *e) override;

private:
    //初始化UI布局
    void initUi();
    //连接槽函数
    void initConnection();

    VNoteIconButton *m_Image;
    DSuggestButton *m_PushButton;
    DLabel *m_Text;
};

#endif // INITEMPTYPAGE_H
