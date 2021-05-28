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
#include "setting.h"

#include <DSettingsOption>
#include <DApplication>

#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

DWIDGET_USE_NAMESPACE

static setting *settinginstance = nullptr;

/**
 * @brief GenerateSettingTranslate
 * 生成翻译文件
 */
void GenerateSettingTranslate()
{
    auto basic = DApplication::translate("Setting", "Basic");
    auto audio_source = DApplication::translate("Setting", "Audio Source");
    auto audio_internal = DApplication::translate("Setting", "Internal");
    auto audio_micphone = DApplication::translate("Setting", "Microphone");
}

/**
 * @brief CustemBackend::CustemBackend
 * @param filepath 参数保存路径
 * @param parent
 */
CustemBackend::CustemBackend(const QString &filepath, QObject *parent)
    : DSettingsBackend(parent)
{
    m_settings = new QSettings(filepath, QSettings::IniFormat);
}

/**
 * @brief CustemBackend::~CustemBackend
 */
CustemBackend::~CustemBackend()
{
    delete m_settings;
}

/**
 * @brief CustemBackend::keys
 * @return 参数的所有key值
 */
QStringList CustemBackend::keys() const
{
    QStringList keyList = m_settings->allKeys();
    for (auto &it : keyList) {
        if (it.indexOf('.') == -1) {
            it.insert(0, "old.");
        }
    }
    return keyList;
}

/**
 * @brief CustemBackend::getOption
 * @param key
 * @return 参数内容
 */
QVariant CustemBackend::getOption(const QString &key) const
{
    if (key.startsWith("old.")) {
        return m_settings->value(key.right(key.length() - 4));
    }
    return m_settings->value(key);
}

/**
 * @brief CustemBackend::doSync
 */
void CustemBackend::doSync()
{
    m_settings->sync();
}

/**
 * @brief CustemBackend::doSetOption
 * @param key 设置的key值
 * @param value 设置的内容
 */
void CustemBackend::doSetOption(const QString &key, const QVariant &value)
{
    m_writeLock.lock();
    if (key.startsWith("old.")) {
        m_settings->setValue(key.right(key.length() - 4), value);
    } else {
        m_settings->setValue(key, value);
    }
    m_settings->sync();
    m_writeLock.unlock();
}

/**
 * @brief setting::setting
 * @param parent
 */
setting::setting(QObject *parent)
    : QObject(parent)
{
    QString vnoteConfigBasePath =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);

    QFileInfo configDir(vnoteConfigBasePath + QDir::separator());

    if (!configDir.exists()) {
        QDir().mkpath(configDir.filePath());
    }

    m_backend = new CustemBackend(configDir.filePath() + QString("config.conf"), this);
    m_setting = DSettings::fromJsonFile(":/deepin-voice-note-setting.json");
    m_setting->setBackend(m_backend);
}

/**
 * @brief setting::setOption
 * @param key 设置的key值
 * @param value 设置的内容
 */
void setting::setOption(const QString &key, const QVariant &value)
{
    if (getOption(key) != value) {
        m_setting->setOption(key, value);
        m_backend->doSetOption(key, value);
    }
}

/**
 * @brief setting::getOption
 * @param key
 * @return 参数内容
 */
QVariant setting::getOption(const QString &key)
{
    return m_setting->getOption(key);
}

/**
 * @brief setting::getSetting
 * @return 参数管理对象
 */
DSettings *setting::getSetting()
{
    return m_setting;
}

/**
 * @brief setting::instance
 * @return 单例对象
 */
setting *setting::instance()
{
    if (settinginstance == nullptr) {
        settinginstance = new setting;
    }
    return settinginstance;
}
