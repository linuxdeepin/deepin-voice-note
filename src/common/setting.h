/*
* Copyright (C) 2019 ~ 2019 UnionTech Software Technology Co.,Ltd.
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
#ifndef SETTING_H
#define SETTING_H

#include <QSettings>
#include <qsettingbackend.h>
#include <DSettings>
#include <DSettingsOption>
#include <QMutex>

DCORE_USE_NAMESPACE

class CustemBackend : public DSettingsBackend
{
   Q_OBJECT
public:
    explicit CustemBackend(const QString &filepath, QObject *parent = nullptr);
    ~CustemBackend() override;
    QStringList keys() const override;
    QVariant getOption(const QString &key) const override;
    void doSync() override;
    void doSetOption(const QString &key, const QVariant &value) override;
private:
    QSettings *m_settings {nullptr};
    QMutex       m_writeLock;
};

class setting : public QObject
{
    Q_OBJECT
public:
    explicit setting(QObject *parent = nullptr);
    void     setOption(const QString &key, const QVariant &value);
    QVariant getOption(const QString &key);
    DSettings  *getSetting();
    static setting* instance();
private:
    CustemBackend *m_backend {nullptr};
    DSettings       *m_setting {nullptr};
};

#endif // SETTING_H
