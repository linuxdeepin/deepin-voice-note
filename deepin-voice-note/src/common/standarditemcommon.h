#ifndef FOLDERTREECOMMON_H
#define FOLDERTREECOMMON_H

#include <QObject>
#include <QStandardItemModel>
class StandardItemCommon : public QObject
{
    Q_OBJECT
public:
    enum StandardItemType {
        Invalid = 0,
        NOTEPADROOT, //记事本一级目录
        NOTEPADITEM, //记事本项
        NOTEITEM    //笔记项
    };
    Q_ENUM(StandardItemType)
    StandardItemCommon();
    static QStandardItem *createStandardItem(void *data, StandardItemType type);
    static StandardItemType getStandardItemType(const QModelIndex &index);
    static void * getStandardItemData(const QModelIndex &index);

};

#endif // FOLDERTREECOMMON_H
