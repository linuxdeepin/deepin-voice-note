#ifndef VOICENOTEITEM_H
#define VOICENOTEITEM_H


#include <DWidget>
#include <DFrame>
#include <DLabel>
#include <DMenu>

DWIDGET_USE_NAMESPACE

struct VNoteItem;
struct VNVoiceBlock;

class TextNoteEdit;
class VNoteIconButton;

class VoiceNoteItem : public DWidget
{
    Q_OBJECT
public:

    explicit VoiceNoteItem(VNoteItem *textNote, VNVoiceBlock *noteBlock, QWidget *parent = nullptr);

    void showPlayBtn();
    void showPauseBtn();
    void showAsrStartWindow();
    void showAsrEndWindow(const QString &strResult);
    void enblePlayBtn(bool enable);
    void enblePauseBtn(bool enable);
    VNoteItem *getNoteItem();
    VNVoiceBlock *getNoteBlock();
signals:
    void voiceMenuShow();
    void sigPlayBtnClicked(VoiceNoteItem *item);
    void sigPauseBtnClicked(VoiceNoteItem *item);

public slots:
    void onPlayBtnClicked();
    void onPauseBtnClicked();
    void onAsrTextChange();
    void onChangeTheme();
protected:
    void mousePressEvent(QMouseEvent *event);
private:
    void initUi();
    void initData();
    void initConnection();
    DLabel          *m_createTimeLab {nullptr};
    DLabel          *m_voiceSizeLab {nullptr};
    DLabel          *m_voiceNameLab {nullptr};
    DFrame          *m_bgWidget {nullptr};
    TextNoteEdit    *m_asrText {nullptr};
    VNoteItem       *m_textNode {nullptr};
    VNVoiceBlock    *m_noteBlock {nullptr};
    VNoteIconButton *m_pauseBtn {nullptr};
    VNoteIconButton *m_playBtn {nullptr};
};

#endif // VOICENOTEITEM_H
