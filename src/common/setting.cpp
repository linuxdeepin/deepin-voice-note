// Copyright (C) 2019-2026 UnionTech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "setting.h"
#include "ddlog.h"

#include <DSettingsOption>
#include <DApplication>

#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

DWIDGET_USE_NAMESPACE

/**
 * @brief GenerateSettingTranslate
 * 生成翻译文件
 */
void GenerateSettingTranslate()
{
    qCInfo(voiceNoteLog) << "Generating setting translations";
    auto basic = DApplication::translate("Setting", "Basic");
    auto audio_source = DApplication::translate("Setting", "Audio Source");
    auto audio_internal = DApplication::translate("Setting", "Internal");
    auto audio_micphone = DApplication::translate("Setting", "Microphone");
    qCInfo(voiceNoteLog) << "Setting translations generated";
}

/**
 * @brief CustemBackend::CustemBackend
 * @param filepath 参数保存路径
 * @param parent
 */
CustemBackend::CustemBackend(const QString &filepath, QObject *parent)
    : DSettingsBackend(parent)
{
    qCInfo(voiceNoteLog) << "CustemBackend constructor called with filepath:" << filepath;
    m_settings = new QSettings(filepath, QSettings::IniFormat);
    qCInfo(voiceNoteLog) << "CustemBackend constructor finished";
}

/**
 * @brief CustemBackend::~CustemBackend
 */
CustemBackend::~CustemBackend()
{
    // qInfo() << "CustemBackend destructor called";
    delete m_settings;
    // qInfo() << "CustemBackend destructor finished";
}

/**
 * @brief CustemBackend::keys
 * @return 参数的所有key值
 */
QStringList CustemBackend::keys() const
{
    qCInfo(voiceNoteLog) << "Getting all keys from settings";
    QStringList keyList = m_settings->allKeys();
    for (auto &it : keyList) {
        if (it.indexOf('.') == -1) {
            it.insert(0, "old.");
        }
    }
    qCInfo(voiceNoteLog) << "Keys retrieval finished, count:" << keyList.size();
    return keyList;
}

/**
 * @brief CustemBackend::getOption
 * @param key
 * @return 参数内容
 */
QVariant CustemBackend::getOption(const QString &key) const
{
    qCInfo(voiceNoteLog) << "Getting option for key:" << key;
    QVariant value;
    if (key.startsWith("old.")) {
        value = m_settings->value(key.right(key.length() - 4));
    } else {
        value = m_settings->value(key);
    }
    qCInfo(voiceNoteLog) << "Option retrieval finished for key:" << key;
    return value;
}

/**
 * @brief CustemBackend::doSync
 */
void CustemBackend::doSync()
{
    qCInfo(voiceNoteLog) << "Syncing settings";
    m_settings->sync();
    qCInfo(voiceNoteLog) << "Settings sync finished";
}

/**
 * @brief CustemBackend::doSetOption
 * @param key 设置的key值
 * @param value 设置的内容
 */
void CustemBackend::doSetOption(const QString &key, const QVariant &value)
{
    qCInfo(voiceNoteLog) << "Setting option, key:" << key << "value:" << value;
    m_writeLock.lock();
    if (key.startsWith("old.")) {
        m_settings->setValue(key.right(key.length() - 4), value);
    } else {
        m_settings->setValue(key, value);
    }
    m_settings->sync();
    m_writeLock.unlock();
    qCInfo(voiceNoteLog) << "Option setting finished for key:" << key;
}

/**
 * @brief setting::setting
 * @param parent
 */
setting::setting()
{
    qCInfo(voiceNoteLog) << "Setting constructor called";
    QString vnoteConfigBasePath =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);

    QFileInfo configDir(vnoteConfigBasePath + QDir::separator());

    if (!configDir.exists()) {
        QDir().mkpath(configDir.filePath());
    }

    m_backend = new CustemBackend(configDir.filePath() + QString("config.conf"), this);
    m_setting = DSettings::fromJsonFile(":/deepin-voice-note-setting.json");
    m_setting->setBackend(m_backend);
    qCInfo(voiceNoteLog) << "Setting constructor finished";
}

/**
 * @brief setting::setOption
 * @param key 设置的key值
 * @param value 设置的内容
 */
void setting::setOption(const QString &key, const QVariant &value)
{
    // qInfo() << "Setting option in setting class, key:" << key << "value:" << value;
    if (getOption(key) != value) {
        m_setting->setOption(key, value);
        m_backend->doSetOption(key, value);
    }
    // qInfo() << "Option setting finished in setting class for key:" << key;
}

/**
 * @brief setting::getOption
 * @param key
 * @return 参数内容
 */
QVariant setting::getOption(const QString &key)
{
    // qInfo() << "Getting option in setting class, key:" << key;
    return m_setting->getOption(key);
}

/**
 * @brief setting::getSetting
 * @return 参数管理对象
 */
DSettings *setting::getSetting()
{
    // qInfo() << "Getting setting object";
    return m_setting;
}

/**
 * @brief setting::instance
 * @return 单例对象
 */
setting *setting::instance()
{
    // qInfo() << "Setting instance requested";
    static setting _instance;
    return &_instance;
}
