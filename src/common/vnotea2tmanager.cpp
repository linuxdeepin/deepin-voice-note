// SPDX-FileCopyrightText: 2019 - 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vnotea2tmanager.h"
#include "opsstateinterface.h"

#include <DSysInfo>

#include <QDBusInterface>
#include <QDBusError>
#include <QJsonParseError>

#include <QTimer>

DCORE_USE_NAMESPACE

// TODO(renbin): 替换新接口
QTimer *timer;

/**
 * @brief VNoteA2TManager::VNoteA2TManager
 * @param parent
 */
VNoteA2TManager::VNoteA2TManager(QObject *parent)
    : QObject(parent)
{
    qDebug() << "Initializing VNoteA2TManager";
    initSession();
}

/**
 * @brief VNoteA2TManager::initSession
 * @return 错误码
 */
int VNoteA2TManager::initSession()
{
    qDebug() << "Initializing ASR session";
    int errorCode = 0;

    timer = new QTimer;
    timer->setSingleShot(true);
    timer->setInterval(100000);
    connect(timer, &QTimer::timeout, [=](){
        emit asrError(AudioOther);
        OpsStateInterface::instance()->operState(OpsStateInterface::StateVoice2Text, false);
    });

    bool ret = QDBusConnection::sessionBus().connect("com.iflytek.aiassistant", "/aiassistant/deepinmain",
                                          "com.iflytek.aiassistant.mainWindow", "onNotify", this, SLOT(onNotify(QString)));
    if (!ret) {
        qWarning() << "Failed to connect to D-Bus signal:" << QDBusConnection::sessionBus().lastError().message();
        errorCode = -1;
    } else {
        qDebug() << "Successfully connected to D-Bus signal";
    }

    return errorCode;
}

/**
 * @brief VNoteA2TManager::startAsr
 * @param filePath 文件路径
 * @param fileDuration 文件时长
 * @param srcLanguage 源语言
 * @param targetLanguage 目标语言
 */
void VNoteA2TManager::startAsr(QString filePath, qint64 fileDuration, QString srcLanguage, QString targetLanguage)
{
    qInfo() << "Starting ASR for file:" << filePath << "duration:" << fileDuration 
            << "source language:" << srcLanguage << "target language:" << targetLanguage;

    QDBusInterface asrInterface("com.iflytek.aiassistant", "/aiassistant/deepinmain",
                                "com.iflytek.aiassistant.mainWindow");
    if (!asrInterface.isValid())
    {
        qWarning() << "Invalid D-Bus interface:" << QDBusConnection::sessionBus().lastError().message();
        emit asrError(AudioOther);
        return;
    }

    QVariantMap param;
    param.insert("filePath", filePath);
    param.insert("fileDuration", fileDuration);

    if (!srcLanguage.isEmpty())
    {
        param.insert("language", srcLanguage);
    }

    if (!targetLanguage.isEmpty())
    {
        param.insert("targetLanguage", targetLanguage);
    }

    qDebug() << "Calling D-Bus startAsr with parameters:" << param;
    QDBusMessage retMessage = asrInterface.call(QLatin1String("startAsr"), param);
    QString retStr = retMessage.arguments().at(0).value<QString>();

    if (retStr != CODE_SUCCESS) {
        qWarning() << "ASR start failed with code:" << retStr;
        asrMsg error;
        error.code = retStr;
        emit asrError(getErrorCode(error));
    } else {
        qDebug() << "ASR started successfully";
        OpsStateInterface::instance()->operState(OpsStateInterface::StateVoice2Text, true);
        timer->start();
    }
}

/**
 * @brief VNoteA2TManager::stopAsr
 */
void VNoteA2TManager::stopAsr()
{
    qInfo() << "Stopping ASR";
    // m_asrInterface->stopAsr();
}

/**
 * @brief VNoteA2TManager::onNotify
 * @param msg 数据信息
 */
void VNoteA2TManager::onNotify(const QString &msg)
{
    qDebug() << "Received ASR notification";
    asrMsg asrData;
    asrJsonParser(msg, asrData);

    if (timer->isActive()) {
        qDebug() << "Stopping ASR timeout timer";
        timer->stop();
    }
    OpsStateInterface::instance()->operState(OpsStateInterface::StateVoice2Text, false);

    if (CODE_SUCCESS == asrData.code)
    {
        qInfo() << "ASR success";
        if (XF_finish == asrData.status)
        {
            qInfo() << "ASR finish";
            // Finish convertion
            emit asrSuccess(asrData.text);
        }
        else if (XF_fail == asrData.status)
        {
            qInfo() << "ASR failed";
            // Failed convertion
            emit asrError(getErrorCode(asrData));
        }
        else
        {
            qInfo() << "ASR other";
            // We ingore other state BCS we don't
            //  care about it
        }
    }
    else
    {
        qInfo() << "ASR error";
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
    qDebug() << "Parsing ASR JSON message";
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8(), &error);

    if (QJsonParseError::NoError == error.error)
    {
        qInfo() << "ASR JSON message is valid";
        QVariantMap map = doc.toVariant().toMap();

        if (map.contains("code"))
        {
            qInfo() << "ASR code is valid";
            asrData.code = map["code"].toString();
        }

        if (map.contains("descInfo"))
        {
            qInfo() << "ASR descInfo is valid";
            asrData.descInfo = map["descInfo"].toString();
        }

        if (map.contains("failType"))
        {
            qInfo() << "ASR failType is valid";
            asrData.failType = map["failType"].toInt();
        }

        if (map.contains("status"))
        {
            qInfo() << "ASR status is valid";
            asrData.status = map["status"].toInt();
        }

        if (map.contains("text"))
        {
            qInfo() << "ASR text is valid";
            asrData.text = map["text"].toString();
        }
        qDebug() << "JSON parsed successfully, code:" << asrData.code << "status:" << asrData.status;
    } else {
        qWarning() << "JSON parse error:" << error.errorString();
    }
}

/**
 * @brief VNoteA2TManager::getErrorCode
 * @param 消息数据
 * @return 错误码
 */
VNoteA2TManager::ErrorCode VNoteA2TManager::getErrorCode(const asrMsg &asrData)
{
    qDebug() << "Getting error code for ASR message, code:" << asrData.code << "status:" << asrData.status;
    ErrorCode error = Success;

    if (CODE_SUCCESS == asrData.code && XF_fail == asrData.status)
    {
        qWarning() << "ASR failed";
        switch (asrData.failType)
        {
        case XF_upload:
        {
            qWarning() << "Audio file upload failed";
            error = UploadAudioFileFail;
        }
        break;
        case XF_decode:
        {
            qWarning() << "Audio decode failed";
            error = AudioDecodeFail;
        }
        break;
        case XF_recognize:
        {
            qWarning() << "Audio recognition failed";
            error = AudioRecognizeFail;
        }
        break;
        case XF_limit:
        {
            qWarning() << "Audio exceeds size limit";
            error = AudioExceedeLimit;
        }
        break;
        case XF_verify:
        {
            qWarning() << "Audio verification failed";
            error = AudioVerifyFailed;
        }
        break;
        case XF_mute:
        {
            qWarning() << "Audio file is mute";
            error = AudioMuteFile;
        }
        break;
        case XF_other:
        {
            qWarning() << "Other audio error occurred";
            error = AudioOther;
        }
        break;

        } // End switch failType
    }
    else if (CODE_NETWORK == asrData.code)
    {
        qWarning() << "Network error occurred";
        error = NetworkError;
    }
    else
    {
        // TODO:
        //     Now we don't care this error,may be handle
        // it in furture if needed
        qDebug() << "Ignoring non-critical error";
        error = DontCareError;
    }
    return error;
}
