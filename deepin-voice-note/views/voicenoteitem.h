#ifndef VOICENOTEITEM_H
#define VOICENOTEITEM_H
#include "common/vnoteitem.h"
#include "myrecodebuttons.h"
#include "textnoteedit.h"
#include "widgets/vnwaveform.h"

#include <DWidget>
#include <DTextEdit>
#include <DFrame>
#include <DLabel>

DWIDGET_USE_NAMESPACE

class VoiceNoteItem : public VNoteItemWidget
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

    bool isPlaying();
    bool isAsrStart();

    VNoteItem *getNoteItem() override;

protected:
    void resizeEvent(QResizeEvent *event) override;

signals:
    void sigTextEditDetail(VNoteItem *textNode, DTextEdit *preTextEdit, const QRegExp &searchKey);
    void sigMenuPopup(VNoteItem *textNode);
    void sigVoicePlayBtnClicked(VoiceNoteItem *item);
    void sigVoicePauseBtnClicked(VoiceNoteItem *item);
    void sigItemAddHeight(int height);

public slots:
    void onPlayBtnClicked();
    void onPauseBtnClicked();
    void onshowMenu();
    void onShowDetail();

    void onTextChanged();
    void onChangeTheme();
private:
    void initUi();
    void initData();
    void initConnection();
    void setTextLineHeight(int value);
    void updateDetilBtn();

    int             m_lastHeight {0};
    bool            m_isBottomSpace {false};

    DLabel          *m_createTimeLab {nullptr};
    DLabel          *m_voiceSizeLab {nullptr};
    DFrame          *m_bgWidget {nullptr};
    VNWaveform      *m_waveForm {nullptr};
    TextNoteEdit    *m_asrText {nullptr};
    VNoteItem       *m_textNode {nullptr};
    MyRecodeButtons *m_pauseBtn {nullptr};
    MyRecodeButtons *m_playBtn {nullptr};
    MyRecodeButtons *m_detailBtn {nullptr};
    MyRecodeButtons *m_menuBtn {nullptr};
};

#endif // VOICENOTEITEM_H
