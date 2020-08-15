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
#include "vndatasafertask.h"
#include "db/vnotesaferoper.h"

#include <DLog>

/**
 * @brief VNDataSaferTask::VNDataSaferTask
 * @param safer 数据对象
 * @param parent
 */
VNDataSaferTask::VNDataSaferTask(const VDataSafer &safer, QObject *parent)
    : VNTask(parent)
    , m_dataSafer(safer)
{
}

/**
 * @brief VNDataSaferTask::run
 */
void VNDataSaferTask::run()
{
    VNoteSaferOper saferOper;

    if (VDataSafer::Safe == m_dataSafer.saferType) {
        saferOper.addSafer(m_dataSafer);
    } else if (VDataSafer::Unsafe == m_dataSafer.saferType) {
        //Normal operation,just remove the safer.
        saferOper.rmSafer(m_dataSafer);
    } else if (VDataSafer::ExceptionSafer == m_dataSafer.saferType) {
        //Exception operation happened ,need clear the data.
        QFileInfo fileInfo(m_dataSafer.path);

        if (fileInfo.exists()) {
            QFile::remove(m_dataSafer.path);
        }

        saferOper.rmSafer(m_dataSafer);
    }

    qInfo() << "Safer task done:" << this
            << " " << m_dataSafer;
}
