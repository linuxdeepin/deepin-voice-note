#ifndef VNOTEMAINWINDOW_H
#define VNOTEMAINWINDOW_H

#include "common/datatypedef.h"
#include "leftview.h"
#include "rightview.h"
#include "initemptypage.h"

#include <DMainWindow>
#include <DSearchEdit>
#include <DSplitter>
#include <DTextEdit>
#include <DStackedWidget>
#include <DLabel>
#include <DFloatingButton>
#include <DAnchors>

DWIDGET_USE_NAMESPACE

class VNoteRecordBar;
class VNoteAudioDeviceWatcher;
class VNoteA2TManager;

class VNoteMainWindow : public DMainWindow
{
    Q_OBJECT
public:
    VNoteMainWindow(QWidget *parent = nullptr);
    virtual ~VNoteMainWindow();
    enum WindowType {WndHomePage, WndNoteShow, WndSearchEmpty, WndTextEdit};
protected:
    void initUI();
    void initData();
    void initConnections();
    void initShortcuts();
    void initTitleBar();
    void initMainView();
    void initLeftView();
    void initRightView();
    void initAudioWatcher();

    void initSpliterView(); //正常主窗口
    void initEmptyFoldersView();//没有记事本的窗口
    void initEmptySearchView(); //没有搜索到记事本窗口
    void initTextEditDetailView(); //文字记录满屏显示窗口

signals:

public slots:
    void onVNoteFoldersLoaded();
    void onVNoteFolderChange(const QModelIndex &previous);
    void onVNoteSearch();
    void onVNoteFolderDel(VNoteFolder *data);
    void onVNoteFolderAdd();
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

private:
    DSearchEdit *m_noteSearchEdit {nullptr};
    DIconButton *m_returnBtn {nullptr};

    DFloatingButton *m_floatingAddBtn {nullptr};
    QWidget         *m_leftViewHolder {nullptr};
    QWidget         *m_rightViewHolder {nullptr};
    QWidget         *m_rightNoteArea {nullptr};

    DSplitter   *m_mainWndSpliter {nullptr};
    LeftView    *m_leftView {nullptr};
    RightView   *m_rightView {nullptr};

    VNoteRecordBar* m_recordBar {nullptr};
    //Audio device state watch thread
    VNoteAudioDeviceWatcher *m_audioDeviceWatcher {nullptr};
    VNoteA2TManager *m_a2tManager {nullptr};

    DTextEdit *m_textEditRightView {nullptr};
    DTextEdit *m_textEditMainWnd{nullptr};
    DLabel *m_labSearchEmpty {nullptr};
    InitEmptyPage *m_wndHomePage {nullptr};
    VNoteItem *m_textNode {nullptr};
    DStackedWidget *m_centerWidget {nullptr};
    QTextCharFormat m_textEditFormat;
    bool            m_isRecording {false};
    bool            m_isAsrVoiceing {false};
};

#endif // VNOTEMAINWINDOW_H
