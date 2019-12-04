#ifndef VNOTEDBMANAGER_H
#define VNOTEDBMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QReadWriteLock>

class VNoteDbManager : public QObject
{
    Q_OBJECT
public:
    explicit VNoteDbManager(QObject *parent = nullptr);
    virtual ~VNoteDbManager();

    static VNoteDbManager* instance();

    int initVNoteDb();

signals:

public slots:

protected:
    QSqlDatabase n_sqlDatabase;
    QSqlQuery m_sqlQuery;

    QReadWriteLock m_rwDbLock;

    static VNoteDbManager* _instance;
};

#endif // VNOTEDBMANAGER_H
