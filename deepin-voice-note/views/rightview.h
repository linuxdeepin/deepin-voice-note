#ifndef RIGHTVIEW_H
#define RIGHTVIEW_H

#include <QVBoxLayout>
#include <QList>
#include <QShortcut>

#include <DWidget>
#include <DDialog>

#include "textnoteedit.h"

DWIDGET_USE_NAMESPACE
struct VNoteItem;
struct VNoteBlock;
struct VNVoiceBlock;

class VoiceNoteItem;
class DetailItemWidget;

class RightView : public DWidget
{
    Q_OBJECT
public:
    explicit RightView(QWidget *parent = nullptr);
    void initData(VNoteItem *data, QRegExp reg = QRegExp());
    DetailItemWidget *insertVoiceItem(const QString &voicePath, qint64 voiceSize);
    DetailItemWidget *insertTextEdit(VNoteBlock *data, bool focus = false, QRegExp reg = QRegExp());
    void  setEnablePlayBtn(bool enable);
    void  delWidget(DetailItemWidget *widget, bool merge = true);
    void  updateData();
    void  mouseMoveSelect(QMouseEvent *event);
    void  selectAllItem();
    void  clearAllSelection();
    void  pasteText();
    void  initAction(DetailItemWidget *widget);
    QString copySelectText(int *start = nullptr, int *end = nullptr); //复制文本
    void cutSelectText(); //剪切文本
    VoiceNoteItem *getVoiceItem(VNoteBlock *data);
    DetailItemWidget *getMenuItem();
signals:
    void sigVoicePlay(VNVoiceBlock *voiceData);
    void sigVoicePause(VNVoiceBlock *voiceData);
    void contentChanged();
    void sigCursorChange(int height, bool mouseMove);
public slots:
    void onTextEditFocusOut();
    void onTextEditDelEmpty();
    void onTextEditTextChange();
    void onVoicePlay(VoiceNoteItem *item);
    void onVoicePause(VoiceNoteItem *item);
protected:
    void leaveEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void saveNote();
    void adjustVerticalScrollBar(QWidget *widget, int defaultHeight);
private:
    void initUi();
    void initMenu();
    void getSelectionCount(int &voiceCount, int &textCount);
    void onMenuShow(DetailItemWidget *widget);

    bool        checkFileExist(const QString &file);
//    <<< <<< < Updated upstream
    DetailItemWidget *getWidgetByPos(const QPoint &pos);
    DetailItemWidget *m_menuWidget {nullptr};
//=======
//    QWidget     *getWidgetByPos(const QPoint &pos);
//    void clearBeforeSelect(const QPoint &);
//private:
//    VoiceNoteItem *m_menuVoice {nullptr};
//>>>>>>> Stashed changes
    VNoteItem   *m_noteItemData {nullptr};
    DetailItemWidget *m_curItemWidget{nullptr};
    DWidget     *m_placeholderWidget {nullptr};
    QVBoxLayout *m_viewportLayout {nullptr};
    //Voice control context menu
    DMenu       *m_noteDetailContextMenu {nullptr};
    DDialog     *m_fileHasDelDialog {nullptr};
    bool         m_fIsNoteModified {false};
//    <<< <<< < Updated upstream
//    == == == =
//    QString m_strSelectText{""};//当前选择的文本内容
//    QList<TextNoteEdit *> m_selectTextItem{nullptr};  //当前选中的包含的textItem
//    QPoint m_pointStart{0, 0}; //选择文本的起始点
//    >>> >>> > Stashed changes
};

#endif // RIGHTVIEW_H
