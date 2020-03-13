#ifndef RIGHTVIEW_H
#define RIGHTVIEW_H

#include <QVBoxLayout>

#include <DWidget>
#include <DDialog>

#include "textnoteedit.h"

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
    DWidget *insertTextEdit(VNoteBlock *data, bool focus = false);
    void  setEnablePlayBtn(bool enable);
    void  delWidget(DWidget *widget);
    void  updateData();
    void  mouseMoveSelect(QMouseEvent *event);
    void  selectAllItem();
    void  clearAllSelection();
    QString getSelectText();
    VoiceNoteItem *getVoiceItem(VNoteBlock *data);
    VoiceNoteItem *getMenuVoiceItem();
    void selectText2Clipboard();
signals:
    void sigVoicePlay(VNVoiceBlock *voiceData);
    void sigVoicePause(VNVoiceBlock *voiceData);
    void contentChanged();
    void sigCursorChange(int height, bool mouseMove);
public slots:
    void onTextEditFocusIn();
    void onTextEditFocusOut();
    void onTextEditDelEmpty();
    void onTextEditTextChange();
    void onVoicePlay(VoiceNoteItem *item);
    void onVoicePause(VoiceNoteItem *item);
    void onVoiceMenuShow();
    void onDltSelectContant();
    void onPaste();
    void onCut();
    void onSelectAll();
protected:
    void leaveEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
//    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
    void saveNote();
    void adjustVerticalScrollBar(QWidget *widget, int defaultHeight);
private:
    void initUi();
    void initMenu();
    bool        checkFileExist(const QString &file);
    QWidget     *getWidgetByPos(const QPoint &pos);
private:
    VoiceNoteItem *m_menuVoice {nullptr};
    VNoteItem   *m_noteItemData {nullptr};
    DWidget     *m_curItemWidget{nullptr};
    DWidget     *m_placeholderWidget {nullptr};
    QVBoxLayout *m_viewportLayout {nullptr};
    //Voice control context menu
    DMenu       *m_voiceContextMenu {nullptr};
    DDialog     *m_fileHasDelDialog {nullptr};
    bool         m_fIsNoteModified {false};
    QString m_strSelectText{""};//当前选择的文本内容
    QList<TextNoteEdit *> m_selectTextItem{nullptr};  //当前选中的包含的textItem
};

#endif // RIGHTVIEW_H
