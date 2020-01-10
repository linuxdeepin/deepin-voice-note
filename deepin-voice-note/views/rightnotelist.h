#ifndef RIGHTNOTELIST_H
#define RIGHTNOTELIST_H
#include "common/datatypedef.h"
#include "voicenoteitem.h"

#include <QSharedPointer>
#include <QList>
#include <DListWidget>
#include <DPushButton>
#include <DTextEdit>

DWIDGET_USE_NAMESPACE

class RightNoteList : public DListWidget
{
    Q_OBJECT
public:
    explicit RightNoteList(QWidget *parent = nullptr);
    void initNoteItem(qint64 folderid, VNOTE_ITEMS_MAP *mapNoteData, const QRegExp &searchKey);
    void addNodeItem(VNoteItem *item, const QRegExp &searchKey, bool isBtnAdd = false);
    void delNodeItem(VNoteItem *item);
    void updateNodeItem(VNoteItem *item);
    VoiceNoteItem *getVoiceItem(VNoteItem *item);

    int  getListHeight();
    qint64 getFolderId();

signals:
    void sigTextEditDetail(VNoteItem *textNode, DTextEdit *preTextEdit, const QRegExp &searchKey);
    void sigDelNote(VNoteItem *textNode);
    void sigMenuPopup(VNoteItem *textNode);
    void sigUpdateNote(VNoteItem *textNode);
    void sigTextEditIsEmpty(VNoteItem *textNode, bool empty);

    void sigVoicePlayBtnClicked(VoiceNoteItem *item);
    void sigVoicePauseBtnClicked(VoiceNoteItem *item);
    void sigListHeightChange();

public slots:
    void onItemAddHeight(int height);

private:
    void initUI();
    void initConnection();
    void adjustWidgetItemWidth(int index);

protected:
    void resizeEvent(QResizeEvent *event);
    qint64 m_forlderId {-1};
};

#endif // RIGHTNOTELIST_H
