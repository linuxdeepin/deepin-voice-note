#ifndef VOICENOTEITEM_H
#define VOICENOTEITEM_H
#include "common/vnoteitem.h"
#include "textnoteedit.h"
#include "widgets/vnoteiconbutton.h"
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
    void setPlayPos(int pos);
    void setSliderEnable(bool enable);

    void enblePlayBtn(bool enable);
    void enblePauseBtn(bool enable);
    void enbleMenuBtn(bool enable);

    VNoteItem *getNoteItem() override;
    void updateData() override;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void enterEvent(QEvent *event) override;

signals:
    void sigTextEditDetail(VNoteItem *textNode, DTextEdit *preTextEdit, const QRegExp &searchKey);
    void sigMenuPopup(VNoteItem *textNode);
    void sigVoicePlayBtnClicked(VoiceNoteItem *item);
    void sigVoicePauseBtnClicked(VoiceNoteItem *item);
    void sigItemAddHeight(int height);
    void sigVoicePlayPosChange(int pos);

public slots:
    void onPlayBtnClicked();
    void onPauseBtnClicked();
    void onshowMenu();
    void onShowDetail();

    void onTextChanged();
    void onChangeTheme();
    void onSliderReleased();
    void onSliderPressed();
private:
    void initUi();
    void initData();
    void initConnection();
    void setTextLineHeight(int value);
    void updateDetilBtn();

    int             m_lastHeight {0};
    bool            m_isBottomSpace {false};
    bool            m_isSliderPressed {false};

    DLabel          *m_createTimeLab {nullptr};
    DLabel          *m_voiceSizeLab {nullptr};
    DLabel          *m_curSizeLab {nullptr};
    DFrame          *m_bgWidget {nullptr};
    VNWaveform      *m_waveForm {nullptr};
    TextNoteEdit    *m_asrText {nullptr};
    VNoteItem       *m_textNode {nullptr};
    VNoteIconButton *m_pauseBtn {nullptr};
    VNoteIconButton *m_playBtn {nullptr};
    VNoteIconButton *m_detailBtn {nullptr};
    VNoteIconButton *m_menuBtn {nullptr};
};

#endif // VOICENOTEITEM_H
