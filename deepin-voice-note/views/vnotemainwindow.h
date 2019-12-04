#ifndef VNOTEMAINWINDOW_H
#define VNOTEMAINWINDOW_H

#include "common/datatypedef.h"

#include <DMainWindow>
#include <DSearchEdit>
#include <DSplitter>

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

signals:

public slots:
    void onVNoteFoldersLoaded();
private:
    DSearchEdit *m_noteSearchEdit {nullptr};

    DSplitter *m_mainWndSpliter {nullptr};
    QWidget   *m_leftViewHolder {nullptr};
    QWidget   *m_rightViewHolder {nullptr};
};

#endif // VNOTEMAINWINDOW_H
