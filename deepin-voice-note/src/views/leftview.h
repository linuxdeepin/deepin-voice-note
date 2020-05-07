#ifndef LEFTVIEW_H
#define LEFTVIEW_H

#include <QStandardItemModel>

#include <DTreeView>
#include <DMenu>
DWIDGET_USE_NAMESPACE

class LeftViewDelegate;
class LeftViewSortFilter;

struct VNoteFolder;

class LeftView : public DTreeView
{
    Q_OBJECT
public:
    explicit LeftView(QWidget *parent = nullptr);
    QStandardItem *getNotepadRoot();

    QModelIndex getNotepadRootIndex();
    QModelIndex setDefaultNotepadItem();
    QModelIndex restoreNotepadItem();

    void setOnlyCurItemMenuEnable(bool enable);
    void addFolder(VNoteFolder* folder);
    void appendFolder(VNoteFolder* folder);
    void editFolder();
    void sort();
    int  folderCount();

    VNoteFolder* removeFolder();

signals:

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent* e) override;
private:
    void initDelegate();
    void initModel();
    void initNotepadRoot();
    void initMenu();
    DMenu               *m_notepadMenu {nullptr};
    QStandardItemModel  *m_pDataModel {nullptr};
    LeftViewDelegate    *m_pItemDelegate {nullptr};
    LeftViewSortFilter  *m_pSortViewFilter {nullptr};
    bool                m_onlyCurItemMenuEnable {false};
};

#endif // LEFTVIEW_H
