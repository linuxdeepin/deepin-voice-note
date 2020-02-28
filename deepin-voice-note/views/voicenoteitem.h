#ifndef VOICENOTEITEM_H
#define VOICENOTEITEM_H


#include <DWidget>
#include <DFrame>
#include <DLabel>

DWIDGET_USE_NAMESPACE

struct VNoteItem;
class TextNoteEdit;
class VNoteIconButton;

class VoiceNoteItem : public DWidget
{
    Q_OBJECT
public:

    explicit VoiceNoteItem(VNoteItem *textNote, QWidget *parent = nullptr);

    void showPlayBtn();
    void showPauseBtn();
    void showAsrStartWindow();
    void showAsrEndWindow(const QString &strResult);
    void enblePlayBtn(bool enable);
    void enblePauseBtn(bool enable);
    VNoteItem *getNoteItem();
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
    void initData();
    void initConnection();
    DLabel          *m_createTimeLab {nullptr};
    DLabel          *m_voiceSizeLab {nullptr};
    DFrame          *m_bgWidget {nullptr};
    TextNoteEdit    *m_asrText {nullptr};
    VNoteItem       *m_textNode {nullptr};
    VNoteIconButton *m_pauseBtn {nullptr};
    VNoteIconButton *m_playBtn {nullptr};
};

#endif // VOICENOTEITEM_H
