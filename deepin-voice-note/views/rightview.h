#ifndef RIGHTVIEW_H
#define RIGHTVIEW_H

#include <QVBoxLayout>

#include <DWidget>
DWIDGET_USE_NAMESPACE
struct VNoteItem;
struct VNoteBlock;
struct VNVoiceBlock;

class VoiceNoteItem;

class RightView : public DWidget
{
    Q_OBJECT
public:
    explicit RightView(QWidget *parent = nullptr);
    void initData(VNoteItem *data);
    DWidget *insertVoiceItem(const QString &voicePath, qint64 voiceSize);
    DWidget *insertTextEdit(VNoteBlock* data, bool focus = false);
    void  setEnablePlayBtn(bool enable);
signals:
    void sigVoicePlay(VNVoiceBlock *voiceData);
    void sigVoicePause(VNVoiceBlock *voiceData);
public slots:
    void onTextEditFocusIn();
    void onTextEditFocusOut();
    void onTextEditDelEmpty();
    void onTextEditTextChange();
    void onVoicePlay(VoiceNoteItem *item);
    void onVoicePause(VoiceNoteItem *item);
    void onVoiceMenu(QAction *action);
protected:
    void leaveEvent(QEvent *event) override;
    void saveNote();
private:
    void        initUi();
    void        delWidget(DWidget *widget);

    VoiceNoteItem *m_menuVoice {nullptr};
    VNoteItem   *m_noteItemData {nullptr};
    DWidget     *m_curItemWidget{nullptr};
    DWidget     *m_viewportWidget {nullptr};
    DWidget     *m_placeholderWidget {nullptr};
    QVBoxLayout *m_viewportLayout {nullptr};
    QScrollArea *m_viewportScrollArea {nullptr};

    //If note is modified
    bool         m_fIsNoteModified {false};
};

#endif // RIGHTVIEW_H
