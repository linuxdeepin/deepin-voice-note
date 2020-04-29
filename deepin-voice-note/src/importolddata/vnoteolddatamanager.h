#ifndef VNOTEOLDDATAMANAGER_H
#define VNOTEOLDDATAMANAGER_H

#include "common/datatypedef.h"
#include "db/vnotedbmanager.h"

#include <QObject>

class OldDataLoadTask;

class VNoteOldDataManager : public QObject
{
    Q_OBJECT
public:
    explicit VNoteOldDataManager(QObject *parent = nullptr);

    static VNoteOldDataManager* instance();
    static void releaseInstance();

    VNOTE_FOLDERS_MAP*   folders();
    VNOTE_ALL_NOTES_MAP* allNotes();

    //Should be called before reqDatas
    void initOldDb();
    void reqDatas();
    void doUpgrade();
signals:
    void dataReady();
    void upgradeFinish();
    void progressValue(int value);
public slots:
    void onFinishLoad();
    void onFinishUpgrade();
    void onProgress(int value);
protected:
    static VNoteOldDataManager* _instance;
    static VNoteDbManager*      m_oldDbManger;

    QScopedPointer<VNOTE_FOLDERS_MAP> m_qspNoteFoldersMap;
    QScopedPointer<VNOTE_ALL_NOTES_MAP> m_qspAllNotes;

    friend class OldDataLoadTask;
};

#endif // VNOTEOLDDATAMANAGER_H
