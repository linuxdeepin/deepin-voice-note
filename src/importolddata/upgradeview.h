/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     V4fr3e <V4fr3e@deepin.io>
*
* Maintainer: V4fr3e <liujinli@uniontech.com>
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
#ifndef UPGRADEVIEW_H
#define UPGRADEVIEW_H

#include <DLabel>
#include <DWaterProgress>

#include <QShowEvent>
#include <QWidget>

DWIDGET_USE_NAMESPACE

class UpgradeView : public QWidget
{
    Q_OBJECT
public:
    explicit UpgradeView(QWidget *parent = nullptr);
    //开始升级
    void startUpgrade();
signals:
    //升级完成
    void upgradeDone();
public slots:
    //设置进度
    void setProgress(int progress);
    //数据从数据库中加载完成，准备升级
    void onDataReady();
    //升级结束
    void onUpgradeFinish();

protected:
    //连接槽函数
    void initConnections();

private:
    DWaterProgress *m_waterProgress {nullptr};
    DLabel *m_tooltipTextLabel {nullptr};
};

#endif // UPGRADEVIEW_H
