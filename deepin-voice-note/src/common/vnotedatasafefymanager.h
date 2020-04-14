#ifndef VNOTEDATASAFEFYMANAGER_H
#define VNOTEDATASAFEFYMANAGER_H

#include "common/datatypedef.h"

#include <QObject>

class VNTaskWorker;
class LoadSafeteyDataWorker;

class VNoteDataSafefyManager : public QObject
{
    Q_OBJECT
public:
    explicit VNoteDataSafefyManager(QObject *parent = nullptr);

    ~VNoteDataSafefyManager();

    static VNoteDataSafefyManager* instance();

    void reqSafers();

    void doSafer(const VDataSafer& safer);
signals:

public slots:
    void onSafersLoaded(SafetyDatas* safers);
protected:
    void initTaskWoker();
protected:
    QScopedPointer<SafetyDatas> m_qsSafetyDatas;

    VNTaskWorker *m_safetyTaskWoker {nullptr};

    LoadSafeteyDataWorker *m_pSaferLoadWorker {nullptr};

    static VNoteDataSafefyManager* _instance;
};

#endif // VNOTEDATASAFEFYMANAGER_H
