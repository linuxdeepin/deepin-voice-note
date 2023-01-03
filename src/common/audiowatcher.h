/*
* Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
*
* Author:     zhangteng <zhangteng@uniontech.com>
* Maintainer: zhangteng <zhangteng@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef AudioWatcher_H
#define AudioWatcher_H

#include <QObject>
#include <QtDBus/QtDBus>

/**
 * @brief AudioService 音频服务名 org.deepin.dde.Audio1
 */
const static QString AudioService = QStringLiteral("org.deepin.dde.Audio1");
//const static QString AudioService = QStringLiteral("com.deepin.daemon.Audio");
/**
 * @brief AudioPath 服务地址 /org/deepin/dde/Audio1
 */
const static QString AudioPath = QStringLiteral("/org/deepin/dde/Audio1");
//const static QString AudioPath = QStringLiteral("/com/deepin/daemon/Audio");
/**
 * @brief AudioInterface 接口名称 org.deepin.dde.Audio1
 */
const static QString AudioInterface = QStringLiteral("org.deepin.dde.Audio1");
//const static QString AudioInterface = QStringLiteral("com.deepin.daemon.Audio");
/**
 * @brief SinkInterface 音频输出接口名称 org.deepin.dde.Audio1.Sink
 */
const static QString SinkInterface = QStringLiteral("org.deepin.dde.Audio1.Sink");
//const static QString SinkInterface = QStringLiteral("com.deepin.daemon.Audio.Sink");
/**
 * @brief SourceInterface 音频输入接口名称 org.deepin.dde.Audio1.Source
 */
const static QString SourceInterface = QStringLiteral("org.deepin.dde.Audio1.Source");
//const static QString SourceInterface = QStringLiteral("com.deepin.daemon.Audio.Source");
/**
 * @brief PropertiesInterface 属性接口名称 org.freedesktop.DBus.Properties
 */
const static QString PropertiesInterface = QStringLiteral("org.freedesktop.DBus.Properties");
/**
 * @brief PropertiesChanged 属性改变信号
 */
const static QString PropertiesChanged = QStringLiteral("PropertiesChanged");

/**
 * @brief The AudioPort class 音频端口
 */
class AudioPort
{
public:
    AudioPort() {}
    friend QDebug operator<<(QDebug argument, const AudioPort &port)
    {
        argument << port.name << port.description << port.availability;

        return argument;
    }

    friend QDBusArgument &operator<<(QDBusArgument &argument, const AudioPort &port)
    {
        argument.beginStructure();
        argument << port.name << port.description << port.availability;
        argument.endStructure();

        return argument;
    }

    friend const QDBusArgument &operator>>(const QDBusArgument &argument, AudioPort &port)
    {

        argument.beginStructure();
        argument >> port.name >> port.description >> port.availability;
        argument.endStructure();
        return argument;
    }

    bool operator==(const AudioPort what) const
    {
        return what.name == name && what.description == description && what.availability == availability;
    }

    bool operator!=(const AudioPort what) const
    {
        return what.name != name || what.description != description || what.availability != availability;
    }
public:
    QString name;
    QString description;
    uchar availability; // 0 for Unknown, 1 for Not Available, 2 for Available.
};


class AudioWatcher : public QObject
{
    Q_OBJECT
public:
    enum AudioMode { Internal,
                     Micphone
                   };
    Q_ENUM(AudioMode)
    explicit AudioWatcher(QObject *parent = nullptr);
    //获取设备名称
    QString getDeviceName(AudioMode mode);
    //获取设备音量
    double getVolume(AudioMode mode);
    //判断设备是否静音
    bool getMute(AudioMode mode);
signals:
    //音量改变信号
    void sigVolumeChange(AudioMode mode);
    //设备改变信号
    void sigDeviceChange(AudioMode mode);
    //静音状态改变信号
    void sigMuteChanged(AudioMode mode);
protected slots:
    //输入设备默认端口改变
    void onDefaultSourceActivePortChanged(AudioPort value);
    //输出设备默认端口改变
    void onDefaultSinkActivePortChanged(AudioPort value);
    //默认输入设备改变
    void onDefaultSourceChanaged(const QDBusObjectPath &value);
    //默认输出设备改变
    void onDefaultSinkChanaged(const QDBusObjectPath &value);
    //默认输入设备音量改变
    void onSourceVolumeChanged(double value);
    //默认输出设备音量改变
    void onSinkVolumeChanged(double value);
    //默认输入设备静音状态改变
    void onSourceMuteChanged(bool value);
    //默认输出设备静音状态改变
    void onSinkMuteChanged(bool value);

    /**
     * @brief onDBusAudioPropertyChanged 音频dbus服务属性改变时触发(注:此槽函数链接的dbus属性改变信号发出时机，是dbus属性已经改变之后才会发出)
     * @param msg
     */
    void onDBusAudioPropertyChanged(QDBusMessage msg);

private:
    //从参数文件中读取设备检测开启标志，无配置文件，默认开启
    //CheckInputDevice=false,关闭检测
    void initWatcherCofing();

    //初始化音频dbus服务的接口
    void initDeviceWacther();
    //连接相关槽函数
    void initConnections();
    /**
     * @brief initDefaultSourceDBusInterface 初始化音频dbus服务默认输入源的接口
     */
    void initDefaultSourceDBusInterface();

    /**
     * @brief initDefaultSourceDBusInterface 初始化音频dbus服务默认输出源的接口
     */
    void initDefaultSinkDBusInterface();

    /**
     * @brief activePortSource 音频dbus服务默认输入源的活跃端口
     * @return
     */
    AudioPort defaultSourceActivePort();

    /**
     * @brief activePortSource 音频dbus服务默认输入源的音量
     * @return
     */
    double defaultSourceVolume();

    /**
     * @brief defaultSourceMute  音频dbus服务默认输入源是否静音
     * @return
     */
    bool defaultSourceMute();

    /**
     * @brief defaultSourceName  音频dbus服务默认输入源名称
     * @return
     */
    QString defaultSourceName();

    /**
     * @brief defaultSinkActivePort 音频dbus服务默认输出源的活跃端口
     * @return
     */
    AudioPort defaultSinkActivePort();

    /**
     * @brief defaultSinkVolume 音频dbus服务默认输出源的音量
     * @return
     */
    double defaultSinkVolume();

    /**
     * @brief defaultSinkMute  音频dbus服务默认输出源是否静音
     * @return
     */
    bool defaultSinkMute();

    /**
     * @brief defaultSourceName  音频dbus服务默认输出源名称
     * @return
     */
    QString defaultSinkName();

private:
    /**
     * @brief 音频服务dbus接口
     * 对应为 (org.deepin.dde.Audio)
     */
    QDBusInterface *m_audioDBusInterface = nullptr;

    /**
     * @brief 音频源(默认音频输入源接口)
     * 对应为 (org.deepin.dde.Audio.Source)
     */
    QDBusInterface *m_defaultSourceDBusInterface = nullptr;

    /**
     * @brief 音频源(默认音频输入源接口)
     * 对应为 (org.deepin.dde.Audio.Sink)
     */
    QDBusInterface *m_defaultSinkDBusInterface = nullptr;
    /**
     * @brief 默认输入音频服务地址 org.deepin.dde.Audio1.SourceX  X---序号
     */
    QString m_defaultSourcePath;

    /**
     * @brief 默认输出音频服务地址 org.deepin.dde.Audio1.SinkX  X---序号
     */
    QString m_defaultSinkPath;

    AudioPort m_outAudioPort;
    AudioPort m_inAudioPort;
    double m_outAudioPortVolume = 0.0;
    double m_inAudioPortVolume = 0.0;
    bool m_fNeedDeviceChecker {true};
    bool m_inAudioMute {false};
    bool m_outAudioMute {false};
};

#endif // AudioWatcher_H
