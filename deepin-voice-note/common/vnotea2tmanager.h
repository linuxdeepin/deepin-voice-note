#ifndef VNOTEA2TMANAGER_H
#define VNOTEA2TMANAGER_H

#include <QObject>

#include <com_iflytek_aiservice_session.h>
#include <com_iflytek_aiservice_asr.h>
struct asrMsg {
    qint32  code;
    QString descInfo;
    qint32  failType;
    qint32  status;
    QString text;
};

class VNoteA2TManager : public QObject
{
    Q_OBJECT
public:
    explicit VNoteA2TManager(QObject *parent = nullptr);

    void initSession();
    /*
     * Reference AIService接口及错误码定义.doc for detail
     *
     *    @filePath     (Needed) Max 5 hours audio file
     *    @fileDuration (Needed) Audio file length in ms
     *    @srcLanguage  (Option) Only support cn, en.Default language=cn
     *    @targetLanguage (Option) Same as srcLanguage
     *
     * Eg: language=cn，targetLanguage=en
     * */
    QString startAsr(QString filePath, int fileDuration,
                     QString srcLanguage = "", QString targetLanguage = "");

    void stopAsr();
signals:
    void asrError(const QString errorCode);
public slots:
    void onNotify(const QString &msg);
protected:
    void asrJsonParser(const QString &msg, asrMsg& asrData);
protected:
    QScopedPointer<com::iflytek::aiservice::session> m_session;
    QScopedPointer<com::iflytek::aiservice::asr>     m_asrInterface;
};

#endif // VNOTEA2TMANAGER_H
