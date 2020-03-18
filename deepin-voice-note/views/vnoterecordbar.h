#ifndef VNOTERECORDBAR_H
#define VNOTERECORDBAR_H

#include <QWidget>
#include <QStackedLayout>

#include <DAnchors>

DWIDGET_USE_NAMESPACE

class VNotePlayWidget;
class VNoteRecordWidget;
class VNoteIconButton;
struct VNVoiceBlock;

class VNoteRecordBar : public QWidget
{
    Q_OBJECT
public:
    explicit VNoteRecordBar(QWidget *parent = nullptr);

    static constexpr int REC_BTN_W = 68;
    static constexpr int REC_BTN_H = 68;

    void initUI();
    void initConnections();
    void cancelRecord();
    void playOrPauseVoice();
    void playVoice(VNVoiceBlock *voiceData);
    void pauseVoice(VNVoiceBlock *voiceData);
    bool stopVoice(VNVoiceBlock *voiceData);
    VNVoiceBlock* getVoiceData();

signals:
    void sigStartRecord();
    void sigFinshRecord(const QString &voicePath,qint64 voiceSize);
    void sigPlayVoice(VNVoiceBlock * voiceData);
    void sigPauseVoice(VNVoiceBlock * voiceData);
    void sigWidgetClose(VNVoiceBlock * voiceData);

public slots:
    void onStartRecord();
    void onFinshRecord(const QString &voicePath,qint64 voiceSize);
    void OnMicrophoneAvailableChanged(int availableState);
    void onClosePlayWidget(VNVoiceBlock *voiceData);

protected:
    bool eventFilter(QObject *o, QEvent *e) override;
    void startRecord();
protected:
    QStackedLayout    *m_mainLayout {nullptr};
    VNotePlayWidget   *m_playPanel {nullptr};
    VNoteRecordWidget *m_recordPanel {nullptr};
    VNoteIconButton   *m_recordBtn {nullptr};
    QWidget           *m_recordBtnHover {nullptr};
    QScopedPointer<DAnchorsBase> m_recBtnAnchor;

    QString          m_recordPath {""};

    //MicrophoneState
    int m_microphoneState {0};
};

#endif // VNOTERECORDBAR_H
