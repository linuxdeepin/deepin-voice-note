#ifndef RIGHTVIEW_H
#define RIGHTVIEW_H

#include <QVBoxLayout>
#include <QList>
#include <QShortcut>
#include <QMovie>
#include <QSettings>

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
    void initData(VNoteItem *data, QRegExp reg = QRegExp(), bool fouse = false);
    DetailItemWidget *insertVoiceItem(const QString &voicePath, qint64 voiceSize);
    DetailItemWidget *insertTextEdit(VNoteBlock *data, bool focus = false, QRegExp reg = QRegExp());
    void  setEnablePlayBtn(bool enable);
    void  delWidget(DetailItemWidget *widget, bool merge = true);
    void  updateData();
    void  mouseMoveSelect(QMouseEvent *event);
    void  selectAllItem();
    void  clearAllSelection();
    void  pasteText();

    void  initAction(DetailItemWidget * widget);
    void  getSelectionCount(int &voiceCount, int &textCount);
    QString copySelectText(); //复制文本
    void cutSelectText(bool isByAction = true); //剪切文本
    void delSelectText();
    void doDelAction(bool isByAction = true);

    void setCurVoicePlay(VoiceNoteItem* item);
    void setCurVoiceAsr(VoiceNoteItem* item);
    void saveMp3();

    VoiceNoteItem *getCurVoicePlay();
    VoiceNoteItem *getCurVoiceAsr();

    DetailItemWidget *getMenuItem();
    bool        hasSelection();
    bool        showDelDialog();

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
    void onPlayUpdate();
    void onChangeTheme();
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
    void initConnection();
    void initMenu();
    void onMenuShow(DetailItemWidget * widget);
    void initAppSetting();

    bool        checkFileExist(const QString &file);
    DetailItemWidget *getWidgetByPos(const QPoint &pos);
    VoiceNoteItem    *m_curPlayItem {nullptr};
    VoiceNoteItem    *m_curAsrItem {nullptr};

    VNoteItem   *m_noteItemData {nullptr};
    DetailItemWidget *m_curItemWidget{nullptr};
    DWidget     *m_placeholderWidget {nullptr};
    QVBoxLayout *m_viewportLayout {nullptr};
    //Voice control context menu
    DMenu       *m_noteDetailContextMenu {nullptr};
    DDialog     *m_fileHasDelDialog {nullptr};
    bool         m_fIsNoteModified {false};
    bool         m_isFristTextChange {false};

    QTimer       *m_playAnimTimer {nullptr};

    //App setting, reference to VNoteApplication's setting
    QSharedPointer<QSettings> m_qspSetting {nullptr};
};

#endif // RIGHTVIEW_H
