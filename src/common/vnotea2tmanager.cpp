// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vnotea2tmanager.h"

#include <DSysInfo>

#include <QDBusError>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QVariantMap>
#include <QApplication>
#include <QDebug>

DCORE_USE_NAMESPACE

/**
 * @brief VNoteA2TManager::VNoteA2TManager
 * @param parent
 */
VNoteA2TManager::VNoteA2TManager(QObject *parent)
    : QObject(parent)
    , m_useNewInterface(false)
{
}

/**
 * @brief VNoteA2TManager::tryInitNewInterface
 * @return 是否成功初始化新接口
 */
bool VNoteA2TManager::tryInitNewInterface()
{
    m_newAsrInterface.reset(new QDBusInterface("com.iflytek.aiassistant",
                                              "/aiassistant/deepinmain",
                                              "com.iflytek.aiassistant.mainWindow",
                                              QDBusConnection::sessionBus()));
    
    if (m_newAsrInterface->isValid()) {
        // 连接信号
        QDBusConnection::sessionBus().connect("com.iflytek.aiassistant",
                                             "/aiassistant/deepinmain",
                                             "com.iflytek.aiassistant.mainWindow",
                                             "onNotify",
                                             this,
                                             SLOT(onNotify(QString)));
        return true;
    }
    
    return false;
}

/**
 * @brief VNoteA2TManager::tryInitOldInterface
 * @return 是否成功初始化旧接口
 */
bool VNoteA2TManager::tryInitOldInterface()
{
    m_session.reset(new QDBusInterface("com.iflytek.aiservice",
                                      "/",
                                      "com.iflytek.aiservice.session",
                                      QDBusConnection::sessionBus()));
    
    if (!m_session->isValid()) {
        return false;
    }
    
    const QString ability("asr");
    const QString appName = qApp->applicationName();
    
    // 调用 createSession 方法
    QDBusReply<QDBusObjectPath> reply = m_session->call("createSession", appName, ability);
    if (!reply.isValid()) {
        return false;
    }
    
    QDBusObjectPath sessionPath = reply.value();
    
    m_asrInterface.reset(new QDBusInterface("com.iflytek.aiservice",
                                           sessionPath.path(),
                                           "com.iflytek.aiservice.asr",
                                           QDBusConnection::sessionBus()));
    
    if (m_asrInterface->isValid()) {
        // 连接信号
        QDBusConnection::sessionBus().connect("com.iflytek.aiservice",
                                             sessionPath.path(),
                                             "com.iflytek.aiservice.asr",
                                             "onNotify",
                                             this,
                                             SLOT(onNotify(QString)));
        return true;
    }
    
    return false;
}

/**
 * @brief VNoteA2TManager::initSession
 * @return 错误码
 */
int VNoteA2TManager::initSession()
{
    // 优先尝试新接口
    if (tryInitNewInterface()) {
        m_useNewInterface = true;
        qInfo() << "Initialized new interface (com.iflytek.aiassistant) successfully";
        return 0;
    }
    
    // 新接口不可用，尝试旧接口
    if (tryInitOldInterface()) {
        m_useNewInterface = false;
        qInfo() << "Initialized old interface (com.iflytek.aiservice) successfully";
        return 0;
    }
    
    // 两个接口都不可用
    qWarning() << "Failed to initialize both new and old interfaces";
    return -1;
}

/**
 * @brief VNoteA2TManager::startAsr
 * @param filePath 文件路径
 * @param fileDuration 文件时长
 * @param srcLanguage 源语言
 * @param targetLanguage 目标语言
 */
void VNoteA2TManager::startAsr(QString filePath,
                               qint64 fileDuration,
                               QString srcLanguage,
                               QString targetLanguage)
{
    int ret = initSession();
    if (ret != 0) {
        emit asrError(AudioOther);
        qInfo() << "initSession failed, errorCode=" << ret;
        return;
    }

    QVariantMap param;
    param.insert("filePath", filePath);
    param.insert("fileDuration", fileDuration);

    if (!srcLanguage.isEmpty()) {
        param.insert("language", srcLanguage);
    }

    if (!targetLanguage.isEmpty()) {
        param.insert("targetLanguage", targetLanguage);
    }

    QString retStr;
    
    // 尝试新接口
    if (m_useNewInterface && m_newAsrInterface) {
        qInfo() << "Using new interface (com.iflytek.aiassistant) for startAsr";
        QDBusReply<QString> reply = m_newAsrInterface->call("startAsr", param);
        if (reply.isValid()) {
            retStr = reply.value();
        } else if (reply.error().type() == QDBusError::UnknownMethod) {
            // 新接口方法不存在，切换到旧接口
            qInfo() << "New interface doesn't support startAsr method, switching to old interface";
            m_useNewInterface = false;
            if (!tryInitOldInterface()) {
                emit asrError(AudioOther);
                return;
            }
        } else {
            qWarning() << "New interface startAsr call failed:" << reply.error().message();
            emit asrError(AudioOther);
            return;
        }
    }
    
    // 使用旧接口
    if (!m_useNewInterface && m_asrInterface) {
        qInfo() << "Using old interface (com.iflytek.aiservice) for startAsr";
        QDBusReply<QString> reply = m_asrInterface->call("startAsr", param);
        if (reply.isValid()) {
            retStr = reply.value();
        } else {
            qWarning() << "Old interface startAsr call failed:" << reply.error().message();
            emit asrError(AudioOther);
            return;
        }
    }
    
    if (retStr.isEmpty()) {
        emit asrError(AudioOther);
        return;
    }

    if (retStr != CODE_SUCCESS) {
        asrMsg error;
        error.code = retStr;
        emit asrError(getErrorCode(error));
    }
}

/**
 * @brief VNoteA2TManager::stopAsr
 */
void VNoteA2TManager::stopAsr()
{
    // 尝试新接口
    if (m_useNewInterface && m_newAsrInterface) {
        qInfo() << "Using new interface (com.iflytek.aiassistant) for stopAsr";
        QDBusReply<void> reply = m_newAsrInterface->call("stopAsr");
        if (!reply.isValid() && reply.error().type() == QDBusError::UnknownMethod) {
            qInfo() << "New interface doesn't support stopAsr method, switching to old interface";
            m_useNewInterface = false;
        } else if (!reply.isValid()) {
            qWarning() << "New interface stopAsr call failed:" << reply.error().message();
        }
    }
    
    // 使用旧接口
    if (!m_useNewInterface && m_asrInterface) {
        qInfo() << "Using old interface (com.iflytek.aiservice) for stopAsr";
        m_asrInterface->call("stopAsr");
    }
}

/**
 * @brief VNoteA2TManager::onNotify
 * @param msg 数据信息
 */
void VNoteA2TManager::onNotify(const QString &msg)
{
    asrMsg asrData;

    asrJsonParser(msg, asrData);

    qInfo() << "msg:" << msg;

    if (CODE_SUCCESS == asrData.code) {
        if (XF_finish == asrData.status) {
            //Finish convertion
            emit asrSuccess(asrData.text);
        } else if (XF_fail == asrData.status) {
            //Failed convertion
            emit asrError(getErrorCode(asrData));
        } else {
            //We ingore other state BCS we don't
            // care about it
        }
    } else {
        emit asrError(getErrorCode(asrData));
    }
}

/**
 * @brief VNoteA2TManager::asrJsonParser
 * @param msg json串
 * @param asrData 解析后的数据
 */
void VNoteA2TManager::asrJsonParser(const QString &msg, asrMsg &asrData)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8(), &error);

    if (QJsonParseError::NoError == error.error) {
        QVariantMap map = doc.toVariant().toMap();

        if (map.contains("code")) {
            asrData.code = map["code"].toString();
        }

        if (map.contains("descInfo")) {
            asrData.descInfo = map["descInfo"].toString();
        }

        if (map.contains("failType")) {
            asrData.failType = map["failType"].toInt();
        }

        if (map.contains("status")) {
            asrData.status = map["status"].toInt();
        }

        if (map.contains("text")) {
            asrData.text = map["text"].toString();
        }
    }
}

/**
 * @brief VNoteA2TManager::getErrorCode
 * @param 消息数据
 * @return 错误码
 */
VNoteA2TManager::ErrorCode VNoteA2TManager::getErrorCode(const asrMsg &asrData)
{
    ErrorCode error = Success;

    if (CODE_SUCCESS == asrData.code && XF_fail == asrData.status) {
        switch (asrData.failType) {
        case XF_upload: {
            error = UploadAudioFileFail;
        } break;
        case XF_decode: {
            error = AudioDecodeFail;
        } break;
        case XF_recognize: {
            error = AudioRecognizeFail;
        } break;
        case XF_limit: {
            error = AudioExceedeLimit;
        } break;
        case XF_verify: {
            error = AudioVerifyFailed;
        } break;
        case XF_mute: {
            error = AudioMuteFile;
        } break;
        case XF_other: {
            error = AudioOther;
        } break;

        } //End switch failType
    } else if (CODE_NETWORK == asrData.code) {
        error = NetworkError;
    } else {
        //TODO:
        //    Now we don't care this error,may be handle
        //it in furture if needed
        error = DontCareError;
    }
    return error;
}
