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

#include "splashview.h"

#include <DLabel>
#include <DWaterProgress>
#include <DApplication>

#include <QVBoxLayout>

DWIDGET_USE_NAMESPACE

/**
 * @brief SplashView::SplashView
 * @param parent
 */
SplashView::SplashView(QWidget *parent)
    : QWidget(parent)
{
    setAutoFillBackground(true);

    auto water = new DWaterProgress(this);
    water->setValue(50);
    water->setTextVisible(false);
    water->start();

    auto label = new DLabel(DApplication::translate("SplashView", "Loading..."));
    label->setObjectName("LoadWidgetLabel");
    label->setForegroundRole(DPalette::TextTitle);

    auto vbox = new QVBoxLayout;
    setLayout(vbox);

    vbox->addStretch();
    vbox->addWidget(water, 0, Qt::AlignCenter);
    vbox->addSpacing(10);
    vbox->addWidget(label, 0, Qt::AlignCenter);
    vbox->addStretch();
}
