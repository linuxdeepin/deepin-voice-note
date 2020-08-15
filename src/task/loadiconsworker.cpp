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
#include "loadiconsworker.h"
#include "common/vnotedatamanager.h"

#include <QPixmap>
#include <QPainter>

QVector<QPixmap> VNoteDataManager::m_defaultIcons[IconsType::MaxIconsType];
QReadWriteLock VNoteDataManager::m_iconLock;

/**
 * @brief LoadIconsWorker::LoadIconsWorker
 * @param parent
 */
LoadIconsWorker::LoadIconsWorker(QObject *parent)
    : VNTask(parent)
{
    //Hold the lock at constructor, this can void
    //other thread acess the icons before that being
    //loaded.
    VNoteDataManager::m_iconLock.lockForWrite();
}

/**
 * @brief LoadIconsWorker::greyPix
 * @param pix
 * @return 置灰后的图标
 */
QPixmap LoadIconsWorker::greyPix(QPixmap pix)
{
    QPixmap temp(pix.size());
    temp.fill(Qt::transparent);

    QPainter iconPainer(&temp);
    iconPainer.setCompositionMode(QPainter::CompositionMode_Source);
    iconPainer.drawPixmap(0, 0, pix);
    iconPainer.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    iconPainer.fillRect(temp.rect(), QColor(0, 0, 0, 64));
    iconPainer.end();

    return temp;
}

/**
 * @brief LoadIconsWorker::run
 */
void LoadIconsWorker::run()
{
    QString defaultIconPathFmt(":/icons/deepin/builtin/default_folder_icons/%1.svg");

    for (int i = 0; i < DEFAULTICONS_COUNT; i++) {
        QString iconPath = defaultIconPathFmt.arg(i + 1);
        QPixmap bitmap(iconPath);
        VNoteDataManager::m_defaultIcons[IconsType::DefaultIcon].push_back(bitmap);
        VNoteDataManager::m_defaultIcons[IconsType::DefaultGrayIcon].push_back(greyPix(bitmap));
    }

    VNoteDataManager::m_iconLock.unlock();
}
