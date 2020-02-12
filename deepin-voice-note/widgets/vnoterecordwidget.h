#ifndef VNOTERECORDWIDGET_H
#define VNOTERECORDWIDGET_H

#include "widgets/vnoteiconbutton.h"
#include "widgets/vnwaveform.h"
#include "common/vnoteaudiomanager.h"

#include <DFloatingWidget>
#include <DLabel>
#include <DWidget>

DWIDGET_USE_NAMESPACE

class VNoteRecordWidget : public DFloatingWidget
{
    Q_OBJECT
public:
    explicit VNoteRecordWidget(QWidget *parent = nullptr);

    void startRecord();
    void cancelRecord();

signals:
    void sigFinshRecord(const QString &voicePath,qint64 voiceSize);

public slots:
    void onPauseRecord();
    void onContinueRecord();
    void onSetMediaSource();
    void onMediaDurationChange(qint64 duration);
    void onAudioBufferProbed(const QAudioBuffer &buffer);

private:
    void initUi();
    void initRecordPath();
    void initRecord();
    void initConnection();
private:
    VNoteIconButton *m_pauseBtn {nullptr};
    VNoteIconButton *m_continueBtn {nullptr};
    VNoteIconButton *m_finshBtn {nullptr};
    DLabel          *m_timeLabel{nullptr};
    DWidget         *m_waveForm {nullptr};
    VNoteAudioManager *m_audioManager{nullptr};
    QString          m_recordDir {""};
    QString          m_recordPath {""};
    qint64           m_recordMsec {0};
};

#endif // VNOTERECORDWIDGET_H
