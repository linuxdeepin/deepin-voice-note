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
class VNote2SIconButton;

//Playing animation interface
class PlayAnimInferface {
public:
    virtual ~PlayAnimInferface();

    virtual void startAnim();
    virtual void stopAnim();
    void setAnimTimer(QTimer *timer);
    virtual void updateAnim() = 0;
protected:
    qint32  m_animPicIndex {0};
    QTimer *m_refreshTimer {nullptr};

    const QVector<QString> m_playBitmap = {
        "play_voice1.svg",
        "play_voice2.svg",
        "play_voice3.svg",
        "play_voice4.svg",
    };
};

class VoiceNoteItem : public DetailItemWidget, public PlayAnimInferface
{
    Q_OBJECT
public:

    explicit VoiceNoteItem(VNoteBlock *noteBlock, QWidget *parent = nullptr);

    void showPlayBtn();
    void showPauseBtn();
    void showAsrStartWindow();
    void showAsrEndWindow(const QString &strResult);
    void enblePlayBtn(bool enable);
    bool asrTextNotEmpty();
    bool isAsrTextPos(const QPoint &globalPos);

    void updateAnim() override;
    void stopAnim() override;

    VNoteBlock *getNoteBlock() override;
    QTextCursor getTextCursor() override;
    void        setTextCursor(const QTextCursor &cursor) override;
    void        updateData() override;
    bool        textIsEmpty() override;
    QRect       getCursorRect() override;
    //选中操作相关
    void selectText(const QPoint &globalPos,QTextCursor::MoveOperation op) override;
    void selectText(QTextCursor::MoveOperation op) override;
    void removeSelectText() override;
    void selectAllText() override;
    void clearSelection() override;
    void setFocus() override;
    bool hasFocus() override;
    bool hasSelection() override;
    bool isSelectAll() override;
    QString getAllText() override;
    QString getSelectText() override;

protected:
    void resizeEvent(QResizeEvent *event) override;

signals:
    void sigPlayBtnClicked(VoiceNoteItem *item);
    void sigPauseBtnClicked(VoiceNoteItem *item);

public slots:
    void onPlayBtnClicked();
    void onAsrTextChange();
    void onChangeTheme();

private:
    void initUi();
    void initConnection();
    bool m_selectAll   {false};
    DLabel          *m_hornLab {nullptr};
    DLabel          *m_createTimeLab {nullptr};
    DLabel          *m_voiceSizeLab {nullptr};
    DLabel          *m_voiceNameLab {nullptr};
    DFrame          *m_bgWidget {nullptr};
    TextNoteEdit    *m_asrText {nullptr};
    VNoteBlock      *m_noteBlock {nullptr};
    VNote2SIconButton *m_playBtn {nullptr};
    DFrame          *m_coverWidget{nullptr};
};

#endif // VOICENOTEITEM_H
