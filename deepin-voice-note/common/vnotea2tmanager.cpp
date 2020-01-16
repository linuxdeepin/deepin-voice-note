#include "vnotea2tmanager.h"

VNoteA2TManager::VNoteA2TManager(QObject *parent)
    : QObject(parent)
{

}

void VNoteA2TManager::initSession()
{
    m_session.reset(new com::iflytek::aiservice::session(
                        "com.iflytek.aiservice",
                        "/",
                        QDBusConnection::sessionBus(), this )
                    );

    const QString ability("asr");
    const QString appName = qApp->applicationName();
    int errorCode = -1;

    QDBusObjectPath qdPath = m_session->createSession(appName, ability, errorCode);

    m_asrInterface.reset(new com::iflytek::aiservice::asr(
                             "com.iflytek.aiservice",
                             qdPath.path(),
                             QDBusConnection::sessionBus(),this )
                         );

    connect(m_asrInterface.get(), &com::iflytek::aiservice::asr::onNotify,
            this, &VNoteA2TManager::onNotify);

}

QString VNoteA2TManager::startAsr(QString filePath,
                                  int fileDuration,
                                  QString srcLanguage,
                                  QString targetLanguage)
{
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

    if (retStr != QString("000000")) {
        emit asrError(retStr);
    }

    return retStr;
}

void VNoteA2TManager::stopAsr()
{
    m_asrInterface->stopAsr();
}

void VNoteA2TManager::onNotify(const QString &msg)
{
    asrMsg asrData;

    asrJsonParser(msg, asrData);

    if (0 == asrData.code) {
        if (4 == asrData.status) {
            //Finish convertion
        }
    }
}

void VNoteA2TManager::asrJsonParser(const QString &msg, asrMsg &asrData)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8(), &error);

    if (QJsonParseError::NoError == error.error) {
        QVariantMap map = doc.toVariant().toMap();

        if (map.contains("code")) {
            asrData.code = map["code"].toInt();
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
