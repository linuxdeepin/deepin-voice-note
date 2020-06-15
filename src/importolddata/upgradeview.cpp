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
#include "upgradeview.h"
#include "importolddata/vnoteolddatamanager.h"
#include "importolddata/olddataloadwokers.h"
#include "importolddata/upgradedbutil.h"
#include "vnoteapplication.h"

#include <QVBoxLayout>

#include <DApplication>
#include <DFontSizeManager>
#include <DLog>

UpgradeView::UpgradeView(QWidget *parent)
    : QWidget(parent)
{
    setAutoFillBackground(true);

    m_waterProgress = new DWaterProgress(this);
    m_waterProgress->start();
    m_waterProgress->setFixedSize(QSize(80,80));

    setProgress(1);

    m_tooltipTextLabel = new DLabel(
                DApplication::translate(
                    "UpgradeView",
                    "Importing notes from the old version, please wait..."),
                this);

    m_tooltipTextLabel->setForegroundRole(DPalette::TextTitle);
    DFontSizeManager::instance()->bind(m_tooltipTextLabel, DFontSizeManager::T4);
    auto vbox = new QVBoxLayout;
    setLayout(vbox);
    vbox->addStretch();
    vbox->addWidget(m_waterProgress, 0, Qt::AlignCenter);
    vbox->addSpacing(10);
    vbox->addWidget(m_tooltipTextLabel, 0, Qt::AlignCenter);
    vbox->addStretch();

    initConnections();

    initAppSetting();
}

void UpgradeView::startUpgrade()
{
    //Update the upgrade state to loading data
    UpgradeDbUtil::saveUpgradeState(*m_qspSetting.get(), UpgradeDbUtil::Loading);

    VNoteOldDataManager::instance()->reqDatas();
}

void UpgradeView::setProgress(int progress)
{
    m_waterProgress->setValue(progress);
}

void UpgradeView::onDataReady()
{
    int foldersCount = VNoteOldDataManager::instance()->folders()->folders.count();

    qInfo() << "Old data ready-->folders:" << foldersCount;
    qInfo() << "Begin upgrade old data to new version";

    if (foldersCount > 0) {
        //Update the upgrade state to Processing.
        UpgradeDbUtil::saveUpgradeState(*m_qspSetting.get(), UpgradeDbUtil::Processing);

        VNoteOldDataManager::instance()->doUpgrade();
    } else {
        qInfo() << "There is no data in old database...stop upgrade.";
        //Update the upgrade state to UpdateDone if no data available.
        UpgradeDbUtil::saveUpgradeState(*m_qspSetting.get(), UpgradeDbUtil::UpdateDone);

        VNoteOldDataManager::instance()->upgradeFinish();
    }
}

void UpgradeView::onUpgradeFinish()
{
    qInfo() << "End upgrade old data to new version.";

    //Update the upgrade state
    UpgradeDbUtil::saveUpgradeState(*m_qspSetting.get(), UpgradeDbUtil::UpdateDone);

    //We don't need Data manager anymore, release it.
    VNoteOldDataManager::releaseInstance();

    //Backup old database.
    UpgradeDbUtil::backUpOldDb();

    //Clear old voice directory.
    UpgradeDbUtil::clearVoices();

    setProgress(100);

    emit upgradeDone();
}

void UpgradeView::initConnections()
{
    connect(VNoteOldDataManager::instance(), &VNoteOldDataManager::dataReady
            , this, &UpgradeView::onDataReady);
    connect(VNoteOldDataManager::instance(), &VNoteOldDataManager::progressValue
            , this, &UpgradeView::setProgress, Qt::QueuedConnection);
    connect(VNoteOldDataManager::instance(), &VNoteOldDataManager::upgradeFinish
            , this, &UpgradeView::onUpgradeFinish, Qt::QueuedConnection);
}

void UpgradeView::initAppSetting()
{
    VNoteApplication* app = reinterpret_cast<VNoteApplication *>(qApp);

    m_qspSetting = app->appSetting();
}