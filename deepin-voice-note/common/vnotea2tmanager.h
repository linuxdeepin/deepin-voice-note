#ifndef VNOTEA2TMANAGER_H
#define VNOTEA2TMANAGER_H

#include <QObject>

#include <com_iflytek_aiservice_session.h>
#include <com_iflytek_aiservice_asr.h>

class VNoteA2TManager : public QObject
{
    Q_OBJECT
public:
    explicit VNoteA2TManager(QObject *parent = nullptr);

signals:

public slots:
protected:
    QScopedPointer<com::iflytek::aiservice::session> m_session;
    QScopedPointer<com::iflytek::aiservice::asr>     m_asrInterface;
};

#endif // VNOTEA2TMANAGER_H
