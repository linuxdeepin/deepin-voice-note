#ifndef FOLDERTREECOMMON_H
#define FOLDERTREECOMMON_H

#include <QObject>
#include <QStandardItemModel>

struct VNoteFolder;
class FolderTreeCommon : public QObject
{
    Q_OBJECT
public:
    enum StandardItemType {
        Invalid = 0,
        NOTEPADROOT, //记事本一级目录
        NOTEPADITEM, //记事本项
    };
    Q_ENUM(StandardItemType)
    FolderTreeCommon();
    static QStandardItem *createStandardItem(void *data, StandardItemType type);
    static StandardItemType getStandardItemType(const QModelIndex &index);
    static void * getStandardItemData(const QModelIndex &index);

};

#endif // FOLDERTREECOMMON_H
