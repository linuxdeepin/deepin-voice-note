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
#include <DScrollArea>

DWIDGET_USE_NAMESPACE

struct VNVoiceBlock;
class VNoteRecordBar;
class VNoteIconButton;
class VNoteAudioDeviceWatcher;
class VNoteA2TManager;
class LeftView;
class MiddleView;
class RightView;
class HomePage;
class SplashView;
class VoiceNoteItem;

class VNoteMainWindow : public DMainWindow
{
    Q_OBJECT
public:
    VNoteMainWindow(QWidget *parent = nullptr);
    virtual ~VNoteMainWindow() override;
    QSharedPointer<QSettings> appSetting() const ;
    bool checkIfNeedExit();

    enum WindowType {
        WndSplashAnim,
        WndHomePage,
        WndNoteShow
    };
    enum SpecialStatus {
        InvalidStatus = 0,
        SearchStart,
        SearchEnd,
        PlayVoiceStart,
        PlayVoiceEnd,
        RecordStart,
        RecordEnd,
        VoiceToTextStart,
        VoiceToTextEnd
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
    void initSplashView();  // Splash animation view
    void initEmptyFoldersView();
    void setSpecialStatus(SpecialStatus status);

    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
signals:

public slots:
    void onVNoteFoldersLoaded();
    void onVNoteFolderChange(const QModelIndex &current, const QModelIndex &previous);
    void onVNoteChange(const QModelIndex &previous);
    void onMenuAction(QAction *action);
    //Process all context menu states
    void onMenuAbout2Show();
    void onVNoteSearch();
    void onCursorChange(int height, bool mouseMove); //调整详情页滚动条
    void onStartRecord();//开始录音
    void onFinshRecord(const QString &voicePath, qint64 voiceSize); //结束录音
    void onRightViewVoicePlay(VNVoiceBlock *voiceData);
    void onRightViewVoicePause(VNVoiceBlock *voiceData);
    void onPlayPlugVoicePlay(VNVoiceBlock *voiceData);
    void onPlayPlugVoicePause(VNVoiceBlock *voiceData);
    void onPlayPlugVoiceStop(VNVoiceBlock *voiceData);

    void onChangeTheme();
    //Audio to text API
    void onA2TStart();
    void onA2TError(int error);
    void onA2TSuccess(const QString &text);
    //Shotcuts slots
    void onPreviewShortcut();
    void initAsrErrMessage();
    void showAsrErrMessage(const QString &strMessage);
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
    void loadNotes(VNoteFolder *folder);
    void loadSearchNotes(const QRegExp &key);

    //详情页视图操作
    void delVoice();
    void voiceTotext();

private:
    DSearchEdit *m_noteSearchEdit {nullptr};

#ifdef TITLE_ACITON_PANEL
    //titlebar actions
    QWidget     *m_actionPanel {nullptr};
    DIconButton *m_addNewNoteBtn {nullptr};
#endif

    DScrollArea     *m_rightViewScrollArea {nullptr};
    QWidget         *m_rightViewHolder {nullptr};
    QWidget         *m_leftViewHolder {nullptr};
    QWidget         *m_middleViewHolder {nullptr};

    DSplitter   *m_mainWndSpliter {nullptr};
    LeftView    *m_leftView {nullptr};
    MiddleView  *m_middleView {nullptr};
    RightView   *m_rightView {nullptr};

    DPushButton *m_addNotepadBtn {nullptr};
    VNoteIconButton *m_addNoteBtn {nullptr};

    VNoteRecordBar *m_recordBar {nullptr};
    //Audio device state watch thread
    VNoteAudioDeviceWatcher *m_audioDeviceWatcher {nullptr};
    VNoteA2TManager *m_a2tManager {nullptr};

    SplashView *m_splashView {nullptr};
    HomePage *m_wndHomePage {nullptr};
    DStackedWidget *m_centerWidget {nullptr};
    bool            m_isRecording {false};
    bool            m_isAsrVoiceing {false};
    bool            m_isPlaying {false};
    bool            m_isExit {false};
    //App setting
    QSharedPointer<QSettings> m_qspSetting {nullptr};
    //Shortcuts key
    QScopedPointer<QShortcut> m_stPreviewShortcuts;
    QRegExp          m_searchKey;
    VoiceNoteItem    *m_currentAsrVoice {nullptr};
    DFloatingMessage *m_asrErrMeassage {nullptr};
    DPushButton      *m_asrAgainBtn {nullptr};
};


#endif // VNOTEMAINWINDOW_H
