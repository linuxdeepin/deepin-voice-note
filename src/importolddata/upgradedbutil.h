#ifndef UPGRADEDBUTIL_H
#define UPGRADEDBUTIL_H
#include "common/datatypedef.h"

#include <QtCore>

class UpgradeDbUtil
{
public:
    UpgradeDbUtil();

    static const QString UPGRADE_STATE;

    enum State {
        Invalid = 0,
        Loading,
        Processing,
        UpdateDone,
    };

    static void saveUpgradeState(QSettings& setting, int state);
    static int  readUpgradeState(QSettings& setting);

    static bool needUpdateOldDb(int state);
    static void checkUpdateState(int state);
    static void backUpOldDb();
    static void clearVoices();

    static void doFolderUpgrade(VNoteFolder* folder);
    static void doFolderNoteUpgrade(qint64 newFolderId, qint64 oldFolderId);
};

#endif // UPGRADEDBUTIL_H
