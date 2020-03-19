#ifndef VOICENOTEITEM_H
#define VOICENOTEITEM_H
#include "common/vnoteitem.h"

#include <QMovie>

#include <DWidget>
#include <DFrame>
#include <DLabel>
#include <DMenu>
#include <DTextEdit>

DWIDGET_USE_NAMESPACE

class TextNoteEdit;
class VNoteIconButton;


class VoiceNoteItem : public DetailItemWidget
{
    Q_OBJECT
public:

    explicit VoiceNoteItem(VNoteBlock *noteBlock, QWidget *parent = nullptr);

    void setHornGif(QMovie *gif);
    void showPlayBtn();
    void showPauseBtn();
    void showAsrStartWindow();
    void showAsrEndWindow(const QString &strResult);
    void enblePlayBtn(bool enable);
    void enblePauseBtn(bool enable);
    bool asrTextNotEmpty();
    bool isAsrTextPos(const QPoint &globalPos);
    bool isAsring();

    VNoteBlock *getNoteBlock();
    QTextCursor getTextCursor();
    void        setTextCursor(const QTextCursor &cursor);
    void        updateData();
    bool        textIsEmpty();
    QRect       getCursorRect();
    //选中操作相关
    void selectText(const QPoint &globalPos,QTextCursor::MoveOperation op);
    void selectText(QTextCursor::MoveOperation op);
    void removeSelectText();
    void selectAllText();
    void clearSelection();
    void setFocus();
    bool hasFocus();
    bool hasSelection();
    QString getAllText();
    QString getSelectText();
signals:
    void sigPlayBtnClicked(VoiceNoteItem *item);
    void sigPauseBtnClicked(VoiceNoteItem *item);

public slots:
    void onPlayBtnClicked();
    void onPauseBtnClicked();
    void onAsrTextChange();
    void onChangeTheme();

private:
    void initUi();
    void initConnection();
    bool m_select   {false};
    QMovie          *m_hornGif {nullptr};
    DLabel          *m_hornLab {nullptr};
    DLabel          *m_createTimeLab {nullptr};
    DLabel          *m_voiceSizeLab {nullptr};
    DLabel          *m_voiceNameLab {nullptr};
    DFrame          *m_bgWidget {nullptr};
    TextNoteEdit    *m_asrText {nullptr};
    VNoteBlock      *m_noteBlock {nullptr};
    VNoteIconButton *m_pauseBtn {nullptr};
    VNoteIconButton *m_playBtn {nullptr};
};

#endif // VOICENOTEITEM_H
