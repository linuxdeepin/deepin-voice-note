#ifndef VNOTEMAINWINDOW_H
#define VNOTEMAINWINDOW_H

#include "common/datatypedef.h"

#include <QShortcut>
#include <QSettings>
#include <QStandardItem>
#include <QList>

#include <DMainWindow>
#include <DSearchEdit>
#include <DSplitter>
#include <DTextEdit>
#include <DStackedWidget>
#include <DLabel>
#include <DFloatingButton>
#include <DAnchors>
#include <DFloatingMessage>

DWIDGET_USE_NAMESPACE

class VNoteRecordBar;
class VNoteIconButton;
class VNoteAudioDeviceWatcher;
class VNoteA2TManager;
class LeftView;
class MiddleView;
class RightView;
class HomePage;

class VNoteMainWindow : public DMainWindow
{
    Q_OBJECT
public:
    VNoteMainWindow(QWidget *parent = nullptr);
    virtual ~VNoteMainWindow() override;
    QSharedPointer<QSettings> appSetting() const ;
    bool checkIfNeedExit();

    enum WindowType {
        WndHomePage,
        WndNoteShow,
        WndSearchEmpty,
        WndTextEdit
    };

protected:
    void initUI();
    void initData();
    void initAppSetting();
    void initConnections();
    void initShortcuts();
    void initTitleBar();
    void initMainView();
    void initLeftView();
    void initMiddleView();
    void initRightView();
    void initAudioWatcher();

    void initSpliterView(); //正常主窗口
    void initEmptyFoldersView();//没有记事本的窗口
    void initEmptySearchView(); //没有搜索到记事本窗口
    void initTextEditDetailView(); //文字记录满屏显示窗口
    void initAsrErrMessage();
    void showAsrErrMessage(const QString &strMessage);

    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
signals:

public slots:
    void onVNoteFoldersLoaded();
    void onVNoteFolderChange(const QModelIndex &current, const QModelIndex &previous);
    void onVNoteChange(const QModelIndex &previous);
    void onAction(QAction *action);
    void onVNoteSearch();
    void onTextEditDetail(VNoteItem *textNode, DTextEdit *preTextEdit, const QRegExp &searchKey);
    void onTextEditReturn();
    void onSearchNoteEmpty(qint64 id);
    void onStartRecord();//开始录音
    void onFinshRecord(const QString &voicePath,qint64 voiceSize); //结束录音
    void onChangeTheme();
    //Audio to text API
    void onA2TStart(const QString& file, qint64 duration);
    void onA2TError(int error);
    void onA2TSuccess(const QString& text);
    //Shotcuts slots
    void onPreviewShortcut();
    void onMenuNoteItemChange();
    void onAsrAgain();
private:
    //左侧列表视图操作相关
    void addNotepad();
    void editNotepad();
    void delNotepad();
    int loadNotepads();

    //中间列表视图操作
    void addNote();
    void editNote();
    void delNote();
    void loadNotes(qint64 id);
    void loadSearchNotes(const QRegExp &key);

private:
    DSearchEdit *m_noteSearchEdit {nullptr};
    DIconButton *m_returnBtn {nullptr};

#ifdef TITLE_ACITON_PANEL
    //titlebar actions
    QWidget     *m_actionPanel {nullptr};
    DIconButton *m_addNewNoteBtn {nullptr};
#endif

    QWidget         *m_rightViewHolder {nullptr};
    QWidget         *m_rightNoteArea {nullptr};
    QWidget         *m_leftViewHolder {nullptr};
    QWidget         *m_middleViewHolder {nullptr};

    DSplitter   *m_mainWndSpliter {nullptr};
    LeftView  *m_leftView {nullptr};
    MiddleView  *m_middleView {nullptr};
    RightView   *m_rightView {nullptr};

    DPushButton *m_addNotepadBtn {nullptr};
    VNoteIconButton *m_addNoteBtn {nullptr};

    VNoteRecordBar* m_recordBar {nullptr};
    //Audio device state watch thread
    VNoteAudioDeviceWatcher *m_audioDeviceWatcher {nullptr};
    VNoteA2TManager *m_a2tManager {nullptr};

    DTextEdit *m_textEditRightView {nullptr};
    DTextEdit *m_textEditMainWnd{nullptr};
    DLabel *m_labSearchEmpty {nullptr};
    HomePage *m_wndHomePage {nullptr};
    VNoteItem *m_textNode {nullptr};
    DStackedWidget *m_centerWidget {nullptr};
    QTextCharFormat m_textEditFormat;
    bool            m_isRecording {false};
    bool            m_isAsrVoiceing {false};
    bool            m_isExit {false};
    //App setting
    QSharedPointer<QSettings> m_qspSetting {nullptr};
    //Shortcuts key
    QScopedPointer<QShortcut> m_stPreviewShortcuts;
    DFloatingMessage *m_asrErrMeassage {nullptr};
    DPushButton     *m_asrAgainBtn {nullptr};
    QRegExp          m_searchKey;
};

#endif // VNOTEMAINWINDOW_H
