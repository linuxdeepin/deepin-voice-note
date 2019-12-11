#include "loadiconsworker.h"
#include "common/vnotedatamanager.h"

#include <QImage>

QVector<QImage> VNoteDataManager::m_defaultIcons;
QReadWriteLock VNoteDataManager::m_iconLock;

LoadIconsWorker::LoadIconsWorker(QObject *parent)
    : QObject(parent)
    , QRunnable ()
{

}

void LoadIconsWorker::run()
{
    QString defaultIconPathFmt(":/icons/deepin/builtin/default_folder_icons/%1.svg");

    VNoteDataManager::m_iconLock.lockForWrite();

    for (int i=0; i<DEFAULTICONS_COUNT; i++) {
        QString iconPath = defaultIconPathFmt.arg(i+1);
        QImage bitmap(iconPath);
        VNoteDataManager::m_defaultIcons.push_back(bitmap);
    }

    VNoteDataManager::m_iconLock.unlock();
}
