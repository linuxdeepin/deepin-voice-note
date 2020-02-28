#ifndef RIGHTVIEW_H
#define RIGHTVIEW_H

#include <QVBoxLayout>

#include <DWidget>
DWIDGET_USE_NAMESPACE
struct VNoteItem;
struct VNoteBlock;
class VoiceNoteItem;

class RightView : public DWidget
{
    Q_OBJECT
public:
    explicit RightView(QWidget *parent = nullptr);
    void initData(VNoteItem *data);
    DWidget *insertVoiceItem(VNoteBlock* data);
    DWidget *insertTextEdit(int preWidgetIndex,VNoteBlock* data, bool focus = false);
public slots:
    void onTextEditFocusIn();
    void onTextEditFocusOut();
    void onTextEditDelEmpty();
    void onTextEditTextChange();
    void onVoicePlay(VoiceNoteItem *item);
    void onVoicePause(VoiceNoteItem *item);
protected:
    void leaveEvent(QEvent *event) override;
    void saveNote();
private:
    void        initUi();
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
