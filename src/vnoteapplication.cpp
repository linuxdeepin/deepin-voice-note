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
#include "vnoteapplication.h"
#include "globaldef.h"
#include "setting.h"

#include <DWidgetUtil>
#include <DGuiApplicationHelper>

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

/**
 * @brief VNoteApplication::VNoteApplication
 * @param argc
 * @param argv
 */
VNoteApplication::VNoteApplication(int &argc, char **argv)
    : DApplication(argc, argv)
{
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::newProcessInstance, this, &VNoteApplication::onNewProcessInstance);
}

/**
 * @brief VNoteApplication::activateWindow
 */
void VNoteApplication::activateWindow()
{
    //Init Normal window at first time
    if (nullptr == m_qspMainWnd.get()) {
        m_qspMainWnd.reset(new VNoteMainWindow());

        m_qspMainWnd->setMinimumSize(DEFAULT_WINDOWS_WIDTH, DEFAULT_WINDOWS_HEIGHT);

        QByteArray mainWindowSize = setting::instance()->getOption(VNOTE_MAINWND_SZ_KEY).toByteArray();

        if (!mainWindowSize.isEmpty()) {
            m_qspMainWnd->restoreGeometry(mainWindowSize);
        }

        //Should be called befor show
        Dtk::Widget::moveToCenter(m_qspMainWnd.get());

        m_qspMainWnd->show();
    } else {
        if (m_qspMainWnd->needShowMax()) {
            m_qspMainWnd->setWindowState(Qt::WindowMaximized);
        } else {
            m_qspMainWnd->setWindowState(Qt::WindowActive);
        }
        m_qspMainWnd->activateWindow();
    }
}

/**
 * @brief VNoteApplication::mainWindow
 * @return 主窗口
 */
VNoteMainWindow *VNoteApplication::mainWindow() const
{
    return m_qspMainWnd.get();
}

/**
 * @brief VNoteApplication::onNewProcessInstance
 * @param pid
 * @param arguments
 */
void VNoteApplication::onNewProcessInstance(qint64 pid, const QStringList &arguments)
{
    Q_UNUSED(pid);
    Q_UNUSED(arguments);

    //TODO:
    //Parase comandline here

    activateWindow();
}

/**
 * @brief VNoteApplication::handleQuitAction
 */
void VNoteApplication::handleQuitAction()
{
    QEvent event(QEvent::Close);
    DApplication::sendEvent(mainWindow(), &event);
}
