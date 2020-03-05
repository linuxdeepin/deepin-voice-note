#ifndef VNOTEPLAYWIDGET_H
#define VNOTEPLAYWIDGET_H

#include <QMediaPlayer>

#include <DFloatingWidget>
#include <DLabel>
#include <DSlider>
#include <DIconButton>
#include <DWidget>

DWIDGET_USE_NAMESPACE

struct VNVoiceBlock;
class VNoteIconButton;
class QPainter;
class QWidget;
class VNotePlayWidget : public DFloatingWidget
{
    Q_OBJECT
public:
    explicit VNotePlayWidget(QWidget *parent = nullptr);
    void setVoiceBlock(VNVoiceBlock * voiceData);
    void pauseVideo();
    void playVideo();
    void stopVideo();
    VNVoiceBlock* getVoiceData();
signals:
    void sigPlayVoice(VNVoiceBlock * voiceData);
    void sigPauseVoice(VNVoiceBlock * voiceData);
    void sigWidgetClose(VNVoiceBlock * voiceData);
public slots:
    void onVoicePlayPosChange(qint64 pos);
    void onSliderValueChange(int value);
    void onPlayBtnClicked();
    void onPauseBtnClicked();
    void onCloseBtnClicked();

protected:
     bool eventFilter(QObject *o, QEvent *e) override;
private:
    void            initUI();
    void            initConnection();
    void            initPlayer();
    DLabel          *m_timeLab  {nullptr};
    DLabel          *m_nameLab  {nullptr};
    DSlider         *m_slider   {nullptr};
    DWidget         *m_sliderHover {nullptr};
    VNoteIconButton *m_pauseBtn {nullptr};
    VNoteIconButton *m_playBtn {nullptr};
    DIconButton     *m_closeBtn {nullptr};
    VNVoiceBlock    *m_voiceBlock {nullptr};
    QMediaPlayer    *m_player {nullptr};
};

#endif // VNOTEPLAYWIDGET_H
