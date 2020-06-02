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

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

#include <DWidgetUtil>
#include <DGuiApplicationHelper>

VNoteApplication::VNoteApplication(int &argc, char **argv)
    : DApplication(argc, argv)
{
    connect(DGuiApplicationHelper::instance()
            ,&DGuiApplicationHelper::newProcessInstance, this
            ,&VNoteApplication::onNewProcessInstance);
}

void VNoteApplication::activateWindow()
{
    //Init Normal window at first time
    if (nullptr == m_qspMainWnd.get()) {
        m_qspMainWnd.reset(new VNoteMainWindow());

        m_qspMainWnd->setMinimumSize(DEFAULT_WINDOWS_WIDTH, DEFAULT_WINDOWS_HEIGHT);

        QByteArray mainWindowSize =
                m_qspSetting->value(VNOTE_MAINWND_SZ_KEY).toByteArray();

        if (!mainWindowSize.isEmpty()) {
            m_qspMainWnd->restoreGeometry(mainWindowSize);
        }

        //Should be called befor show
        Dtk::Widget::moveToCenter(m_qspMainWnd.get());

        m_qspMainWnd->show();
    } else {
        m_qspMainWnd->setWindowState(Qt::WindowActive);
        m_qspMainWnd->activateWindow();
    }
}

void VNoteApplication::initAppSetting()
{
    QString vnoteConfigBasePath =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);

    QFileInfo configDir(vnoteConfigBasePath+QDir::separator());

    //TODO:
    //    Create app's config dir.
    if (!configDir.exists()) {
        QDir().mkpath(configDir.filePath());
        qInfo() << "Create config dir:" << configDir.filePath();
    }

    m_qspSetting.reset(new QSettings(configDir.filePath() + QString("config.conf")
                                     , QSettings::IniFormat));
}

QSharedPointer<QSettings> VNoteApplication::appSetting() const
{
    return m_qspSetting;
}

VNoteMainWindow *VNoteApplication::mainWindow() const
{
    return m_qspMainWnd.get();
}

void VNoteApplication::onNewProcessInstance(qint64 pid, const QStringList &arguments)
{
    Q_UNUSED(pid);
    Q_UNUSED(arguments);

    //TODO:
    //Parase comandline here

    activateWindow();
}

void VNoteApplication::handleQuitAction()
{
    QEvent event(QEvent::Close);
    DApplication::sendEvent(mainWindow(), &event);
}
