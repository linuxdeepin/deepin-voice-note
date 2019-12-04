#include "db/vnotedbmanager.h"

VNoteDbManager* VNoteDbManager::_instance = nullptr;

VNoteDbManager::VNoteDbManager(QObject *parent)
    : QObject(parent)
{

}

VNoteDbManager::~VNoteDbManager()
{
}

VNoteDbManager *VNoteDbManager::instance()
{
    if (nullptr != _instance) {
        _instance = new VNoteDbManager();
    }

    return  _instance;
}
