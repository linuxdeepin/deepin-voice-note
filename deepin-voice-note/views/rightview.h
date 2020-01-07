#ifndef RIGHTVIEW_H
#define RIGHTVIEW_H
#include "common/vnoteforlder.h"
#include "rightnotelist.h"
#include "myrecodebuttons.h"

#include <QSharedPointer>

#include <DPushButton>
#include <DStackedWidget>
#include <DLabel>

DWIDGET_USE_NAMESPACE

class RightView : public DWidget
{
    Q_OBJECT
public:
    explicit RightView(QWidget *parent = nullptr);
    void noteSwitchByFolder(qint64 id);
    void noteDelByFolder(qint64 id);
    void setSearchKey(const QRegExp &searchKey);
    QList<qint64> getNoteContainsKeyFolders(const QRegExp &searchKey);

signals :
    void sigTextEditDetail(VNoteItem *textNode, DTextEdit *preTextEdit, const QRegExp &searchKey);
    void sigSeachEditFocus();
    void sigSearchNoteEmpty(qint64 id);
protected:
    void resizeEvent(QResizeEvent *event);
private:
    void initUi();
    void initConnection();
    void adjustaddTextBtn();
    void addNewNoteList(qint64 id);

public slots:
    void onAddNote();
    void onDelNote(VNoteItem *item);
    void onUpdateNote(VNoteItem *item);
    void onTextEditIsEmpty(VNoteItem *textNode, bool empty);
private:
    DPushButton  *m_addTextBtn {nullptr};
    DStackedWidget *m_stackWidget {nullptr};
    QRegExp m_searchKey;
    RightNoteList *m_searchNoteList{nullptr};
    MyRecodeButtons *m_addVoiceBtn;
    DLabel *m_noSearchResult {nullptr};
    qint64 m_lastFolderId {-1};
};

#endif // RIGHTVIEW_H
