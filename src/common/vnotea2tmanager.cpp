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
#include "vnotea2tmanager.h"

#include <QDBusError>

#include <DSysInfo>

DCORE_USE_NAMESPACE

VNoteA2TManager::VNoteA2TManager(QObject *parent)
    : QObject(parent)
{

}

bool VNoteA2TManager::checkAiService() const
{

    bool fAiServiceExist = true;

    com::iflytek::aiservice::session session(
                           "com.iflytek.aiservice",
                           "/",
                           QDBusConnection::sessionBus()
                       );

    const QString ability("asr");
    const QString appName = qApp->applicationName()+QString("-CheckAiService");
    int errorCode = -1;

    QString systemInfo = QString("[%1-%2]")
            .arg(DSysInfo::operatingSystemName())
            .arg(DSysInfo::deepinTypeDisplayName());

    QDBusReply<QDBusObjectPath> qdPath = session.createSession(appName, ability, errorCode);

    if (!qdPath.isValid()) {
        if (QDBusError::ServiceUnknown == qdPath.error().type()) {
            fAiServiceExist = false;
            qInfo() << "Aiservice don't exist in system:" << systemInfo << " errorCode=" << errorCode;
        }
    } else {
        qInfo() << "Aiservice exist in system:" << systemInfo << " errorCode=" << errorCode;
    }

    //Free the test session
    session.freeSession(appName, ability);

    return fAiServiceExist;
}

int VNoteA2TManager::initSession()
{
    m_session.reset(new com::iflytek::aiservice::session(
                        "com.iflytek.aiservice",
                        "/",
                        QDBusConnection::sessionBus() )
                    );

    const QString ability("asr");
    const QString appName = qApp->applicationName();
    int errorCode = -1;

    QDBusObjectPath qdPath = m_session->createSession(appName, ability, errorCode);

    m_asrInterface.reset(new com::iflytek::aiservice::asr(
                             "com.iflytek.aiservice",
                             qdPath.path(),
                             QDBusConnection::sessionBus() )
                         );

    connect(m_asrInterface.get(), &com::iflytek::aiservice::asr::onNotify,
            this, &VNoteA2TManager::onNotify);

    return errorCode;

}

void VNoteA2TManager::startAsr(QString filePath,
                                  qint64 fileDuration,
                                  QString srcLanguage,
                                  QString targetLanguage)
{
    int ret = initSession();
    if(ret != 0){
        emit asrError(AudioOther);
        qInfo() << "createSession->errorCode=" << ret;
        return;
    }

    QVariantMap param;

    param.insert("filePath",filePath);
    param.insert("fileDuration",fileDuration);

    if (!srcLanguage.isEmpty()) {
        param.insert("language", srcLanguage);
    }

    if (!targetLanguage.isEmpty()) {
        param.insert("targetLanguage", targetLanguage);
    }

    QString retStr = m_asrInterface->startAsr(param);

    if (retStr != CODE_SUCCESS) {
        asrMsg error;
        error.code = retStr;
        emit asrError(getErrorCode(error));
    }
}

void VNoteA2TManager::stopAsr()
{
    m_asrInterface->stopAsr();
}

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
    } else if (CODE_NETWORK ==asrData.code) {
        error = NetworkError;
    } else {
        //TODO:
        //    Now we don't care this error,may be handle
        //it in furture if needed
        error = DontCareError;
    }
    return error;
}
