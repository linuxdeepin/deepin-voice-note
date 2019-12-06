#ifndef VNOTEMAINWINDOW_H
#define VNOTEMAINWINDOW_H

#include "common/datatypedef.h"
#include "leftview.h"
#include "rightview.h"

#include <DMainWindow>
#include <DSearchEdit>
#include <DSplitter>
#include <DPushButton>

DWIDGET_USE_NAMESPACE

class VNoteMainWindow : public DMainWindow
{
    Q_OBJECT
public:
    VNoteMainWindow(QWidget *parent = nullptr);
    virtual ~VNoteMainWindow();

protected:
    void initUI();
    void initData();
    void initConnections();
    void initShortcuts();
    void initTitleBar();
    void initMainView();
    void initLeftView();
    void initRightView();
    void resizeEvent(QResizeEvent *event) override;

signals:

public slots:
    void onVNoteFoldersLoaded();
    void onVNoteFolderChange(const QModelIndex &previous);
    void onVNoteSearch();
private:
    DSearchEdit *m_noteSearchEdit {nullptr};

    DSplitter *m_mainWndSpliter {nullptr};
    LeftView   *m_leftViewHolder {nullptr};
    RightView   *m_rightViewHolder {nullptr};
    DPushButton *m_btnAddFoler {nullptr};
};

#endif // VNOTEMAINWINDOW_H
