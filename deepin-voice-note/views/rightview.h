#ifndef RIGHTVIEW_H
#define RIGHTVIEW_H
#include "common/vnoteforlder.h"
#include "rightnotelist.h"
#include "myrecodebuttons.h"

#include <QSharedPointer>
#include <QMediaPlayer>

#include <DPushButton>
#include <DStackedWidget>
#include <DLabel>
#include <DMenu>

DWIDGET_USE_NAMESPACE

class RightView : public DWidget
{
    Q_OBJECT
public:
    explicit RightView(QWidget *parent = nullptr);
    void noteSwitchByFolder(qint64 id);
    void noteDelByFolder(qint64 id);
    void setSearchKey(const QRegExp &searchKey);
    void addVoiceNoteItem(const QString &voicePath,qint64 voiceSize);
    void setVoicePlayEnable(bool enable);

    QList<qint64> getNoteContainsKeyFolders(const QRegExp &searchKey);

signals :
    void sigTextEditDetail(VNoteItem *textNode, DTextEdit *preTextEdit, const QRegExp &searchKey);
    void sigSeachEditFocus();
    void sigSearchNoteEmpty(qint64 id);
protected:
    void resizeEvent(QResizeEvent *event);
private:
    void initUi();
    void initTextMenu();
    void initVoiceMenu();
    void initConnection();
    void addNewNote(VNoteItem &data,bool isByBtn = false);
    void addNewNoteList(qint64 id);
    void delNoteFromList(VNoteItem *item,RightNoteList *list);
    void stopCurVoicePlaying(int pos = -1);
    RightNoteList *getNormalNoteList(qint64);


public slots:
    void adjustaddTextBtn();
    void onAddNote();
    void onMenuPopup(VNoteItem *item);
    void onDelNote(VNoteItem *item);
    void onUpdateNote(VNoteItem *item);
    void onTextEditIsEmpty(VNoteItem *textNode, bool empty);

    void onSaveTextAction();
    void onDelTextAction();
    void onSaveVoiceAction();
    void onDelVoiceAction();
    void onAsrVoiceAction();
    void onVoicePlayBtnClicked(VoiceNoteItem *item);
    void onVoicePauseBtnClicked(VoiceNoteItem *item);
    void onVoicePlayPosChange(qint64 pos);
    void onSetPlayerPos(int pos);
private:
    DPushButton  *m_addTextBtn {nullptr};
    DStackedWidget *m_stackWidget {nullptr};
    QRegExp m_searchKey;
    RightNoteList *m_searchNoteList{nullptr};
    DLabel *m_noSearchResult {nullptr};
    qint64 m_lastFolderId {-1};

    DMenu   *m_textMenu {nullptr};
    QAction *m_saveTextAction {nullptr};
    QAction *m_delTextAction{nullptr};

    DMenu   *m_voiceMenu {nullptr};
    QAction *m_saveVoicetAction {nullptr};
    QAction *m_delVoiceAction{nullptr};
    QAction *m_asrVoiceAction{nullptr};

    VNoteItem *m_curNoteItem{nullptr};

    VoiceNoteItem *m_playVoiceItem {nullptr}; //操作的语音项
    QMediaPlayer *m_player {nullptr};
};

#endif // RIGHTVIEW_H
