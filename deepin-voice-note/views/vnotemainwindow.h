#ifndef VNOTEMAINWINDOW_H
#define VNOTEMAINWINDOW_H

#include "common/datatypedef.h"
#include "leftview.h"
#include "rightview.h"
#include "myrecodebuttons.h"
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

    void initSpliterView(); //正常主窗口
    void initEmptyFoldersView();//没有记事本的窗口
    void initEmptySearchView(); //没有搜索到记事本窗口
    void initTextEditDetailView(); //文字记录满屏显示窗口
    void switchView(WindowType type); //显示窗口

signals:

public slots:
    void onVNoteFoldersLoaded();
    void onVNoteFolderChange(const QModelIndex &previous);
    void onVNoteSearch();
    void onVNoteFolderDel(VNoteFolder *data);
    void onVNoteFolderAdd();
    void onTextEditDetail(VNoteItem *textNode, DTextEdit *preTextEdit, const QString &searchKey);
    void onTextEditReturn();
    void onSearchEditFocus();

private:
    DSearchEdit *m_noteSearchEdit {nullptr};
    DIconButton *m_returnBtn {nullptr};

    DFloatingButton *m_floatingAddBtn {nullptr};
    QWidget         *m_leftViewHolder{nullptr};

    DSplitter *m_mainWndSpliter {nullptr};
    LeftView   *m_leftView {nullptr};
    RightView   *m_rightViewHolder {nullptr};

    MyRecodeButtons *m_btnHideTextEdit {nullptr};
    DTextEdit *m_textEditRightView {nullptr};
    DTextEdit *m_textEditMainWnd{nullptr};
    DLabel *m_labSearchEmpty {nullptr};
    InitEmptyPage *m_wndHomePage {nullptr};
    VNoteItem *m_textNode {nullptr};
    DStackedWidget *m_centerWidget {nullptr};
    QTextCharFormat m_textEditFormat;
};

#endif // VNOTEMAINWINDOW_H
