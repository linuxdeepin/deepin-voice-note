#ifndef RIGHTVIEW_H
#define RIGHTVIEW_H

#include <QVBoxLayout>
#include <QList>
#include <QShortcut>
#include <QMovie>
#include <QSettings>
#include <QMultiMap>
#include <QList>

#include <DWidget>
#include <DDialog>

#include "textnoteedit.h"

DWIDGET_USE_NAMESPACE
struct VNoteFolder;
struct VNoteItem;
struct VNoteBlock;
struct VNVoiceBlock;

class VoiceNoteItem;
class DetailItemWidget;


enum ItemWidgetType{VoicePlugin,TextEditPlugin};
typedef  QMultiMap<ItemWidgetType,DetailItemWidget*> MultiMapWidget;

class RightView : public DWidget
{
    Q_OBJECT
public:
    explicit RightView(QWidget *parent = nullptr);
    void initData(VNoteItem *data, QString reg, bool fouse = false);
    DetailItemWidget *insertVoiceItem(const QString &voicePath, qint64 voiceSize);
    DetailItemWidget *insertTextEdit(VNoteBlock *data, bool focus = false,
                                     QTextCursor::MoveOperation op = QTextCursor::NoMove,
                                     QString reg = "");

    void  setEnablePlayBtn(bool enable);
    void  delWidget(DetailItemWidget *widget, bool merge = true);
    void  saveNote();
    void  updateData();
    void  mouseMoveSelect(QMouseEvent *event);
    void  selectAllItem();
    void  clearAllSelection();
    void  pasteText();
    void  removeSelectWidget(DetailItemWidget *widget);

    int   initAction(DetailItemWidget *widget);

    bool isAllWidgetEmpty(const QList<DetailItemWidget*> &widget);
    DetailItemWidget *getOnlyOneSelectVoice();

    QString getSelectText(bool voiceText = true);

    void copySelectText(bool voiceText = true);
    void cutSelectText(); //剪切文本
    void delSelectText();

    void setCurVoicePlay(VoiceNoteItem *item);
    void setCurVoiceAsr(VoiceNoteItem *item);
    void saveMp3();
    void removeCacheWidget(VNoteItem *data);
    void removeCacheWidget(const VNoteFolder *data);

    VoiceNoteItem *getCurVoicePlay();
    VoiceNoteItem *getCurVoiceAsr();
    int         showWarningDialog();

signals:
    void sigVoicePlay(VNVoiceBlock *voiceData);
    void sigVoicePause(VNVoiceBlock *voiceData);
    void contentChanged();
    void sigCursorChange(int height, bool mouseMove);
public slots:
    void onTextEditFocusOut();
    void onTextEditTextChange();
    void onTextEditSelectChange();
    void onVoicePlay(VoiceNoteItem *item);
    void onVoicePause(VoiceNoteItem *item);
    void onPlayUpdate();
protected:
    void leaveEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void adjustVerticalScrollBar(QWidget *widget, int defaultHeight);
private:
    void initUi();
    void initConnection();
    void initMenu();
    void onMenuShow(DetailItemWidget *widget);
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

    MultiMapWidget m_selectWidget;
    QString        m_searchKey {""};

    QMap<VNoteBlock*, DetailItemWidget*> m_mapWidgetCache;
};

#endif // RIGHTVIEW_H
