// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "upgradeview.h"
#include "importolddata/vnoteolddatamanager.h"
#include "importolddata/olddataloadwokers.h"
#include "importolddata/upgradedbutil.h"
#include "vnoteapplication.h"
#include "setting.h"

#include <DApplication>
#include <DFontSizeManager>
#include <DLog>

#include <QVBoxLayout>
#include <QDebug>

/**
 * @brief UpgradeView::UpgradeView
 * @param parent
 */
UpgradeView::UpgradeView(QWidget *parent)
    : QWidget(parent)
{
    qInfo() << "UpgradeView constructor called";
    setAutoFillBackground(true);

    m_waterProgress = new DWaterProgress(this);
    m_waterProgress->start();
    m_waterProgress->setFixedSize(QSize(80, 80));

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
    qInfo() << "UpgradeView constructor finished";
}

/**
 * @brief UpgradeView::startUpgrade
 */
void UpgradeView::startUpgrade()
{
    qInfo() << "Starting upgrade process";
    //Update the upgrade state to loading data
    UpgradeDbUtil::saveUpgradeState(UpgradeDbUtil::Loading);

    VNoteOldDataManager::instance()->reqDatas();
    qInfo() << "Upgrade process start request sent";
}

/**
 * @brief UpgradeView::setProgress
 * @param progress
 */
void UpgradeView::setProgress(int progress)
{
    qInfo() << "Setting progress to:" << progress;
    m_waterProgress->setValue(progress);
    qInfo() << "Progress setting finished";
}

/**
 * @brief UpgradeView::onDataReady
 */
void UpgradeView::onDataReady()
{
    qInfo() << "Old data is ready";
    int foldersCount = VNoteOldDataManager::instance()->folders()->folders.count();

    qInfo() << "Old data ready - Found" << foldersCount << "folders";
    qInfo() << "Beginning upgrade of old data to new version";

    if (foldersCount > 0) {
        qDebug() << "Starting data upgrade process";
        //Update the upgrade state to Processing.
        UpgradeDbUtil::saveUpgradeState(UpgradeDbUtil::Processing);

        VNoteOldDataManager::instance()->doUpgrade();
    } else {
        qInfo() << "No data found in old database - skipping upgrade";
        //Update the upgrade state to UpdateDone if no data available.
        UpgradeDbUtil::saveUpgradeState(UpgradeDbUtil::UpdateDone);

        VNoteOldDataManager::instance()->upgradeFinish();
    }
    qInfo() << "Data ready handling finished";
}

/**
 * @brief UpgradeView::onUpgradeFinish
 */
void UpgradeView::onUpgradeFinish()
{
    qInfo() << "Database upgrade process completed";

    //Update the upgrade state
    UpgradeDbUtil::saveUpgradeState(UpgradeDbUtil::UpdateDone);

    //We don't need Data manager anymore, release it.
    VNoteOldDataManager::releaseInstance();

    //Backup old database.
    UpgradeDbUtil::backUpOldDb();

    //Clear old voice directory.
    UpgradeDbUtil::clearVoices();

    setProgress(100);

    emit upgradeDone();
    qInfo() << "Upgrade finish handling completed";
}

/**
 * @brief UpgradeView::initConnections
 */
void UpgradeView::initConnections()
{
    qInfo() << "Initializing upgrade view connections";
    connect(VNoteOldDataManager::instance(), &VNoteOldDataManager::dataReady, this, &UpgradeView::onDataReady);
    connect(VNoteOldDataManager::instance(), &VNoteOldDataManager::progressValue, this, &UpgradeView::setProgress, Qt::QueuedConnection);
    connect(VNoteOldDataManager::instance(), &VNoteOldDataManager::upgradeFinish, this, &UpgradeView::onUpgradeFinish, Qt::QueuedConnection);
    qInfo() << "Upgrade view connections initialized";
}
