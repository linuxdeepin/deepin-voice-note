#include "vnotedatasafefymanager.h"
#include "vntaskworker.h"
#include "task/loadsafeteydataworker.h"
#include "task/vndatasafertask.h"

#include <QThreadPool>

#include <DLog>

VNoteDataSafefyManager* VNoteDataSafefyManager::_instance = nullptr;

VNoteDataSafefyManager::VNoteDataSafefyManager(QObject *parent)
    : QObject(parent)
{
    initTaskWoker();
}

VNoteDataSafefyManager::~VNoteDataSafefyManager()
{
    m_safetyTaskWoker->quitWorker();
}

VNoteDataSafefyManager *VNoteDataSafefyManager::instance()
{
    if (nullptr == _instance) {
        _instance = new VNoteDataSafefyManager();
    }

    return _instance;
}

void VNoteDataSafefyManager::reqSafers()
{
    m_pSaferLoadWorker = new LoadSafeteyDataWorker();
    m_pSaferLoadWorker->setAutoDelete(true);

    //Need direct call here.
    connect(m_pSaferLoadWorker, &LoadSafeteyDataWorker::saferLoaded,
            this, &VNoteDataSafefyManager::onSafersLoaded, Qt::DirectConnection);

    QThreadPool::globalInstance()->start(m_pSaferLoadWorker);
}

void VNoteDataSafefyManager::doSafer(const VDataSafer &safer)
{
    if (safer.isValid()) {
        VNDataSaferTask* pSafeTask = new VNDataSaferTask(safer);

        Q_ASSERT(nullptr != m_safetyTaskWoker);

        m_safetyTaskWoker->addTask(pSafeTask);
    }
}

void VNoteDataSafefyManager::onSafersLoaded(SafetyDatas *safers)
{
    if (m_qsSafetyDatas != nullptr) {
        m_qsSafetyDatas->clear();
    }

    m_qsSafetyDatas.reset(safers);

    qInfo() << "Safer loaded ok:" << m_qsSafetyDatas->size();

    //Add invalid data clearing task to VNTaskWorker
    for (auto it : *m_qsSafetyDatas.get()) {
        doSafer(it);
    }

    m_pSaferLoadWorker = nullptr;
}

void VNoteDataSafefyManager::initTaskWoker()
{
    m_safetyTaskWoker = new VNTaskWorker();
    m_safetyTaskWoker->setWorkerName("VNSaferWorker");

    m_safetyTaskWoker->start();
}
