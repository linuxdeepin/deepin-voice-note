#ifndef LEFTVIEW_H
#define LEFTVIEW_H

#include <QStandardItemModel>

#include <DTreeView>
#include <DMenu>
DWIDGET_USE_NAMESPACE

class LeftViewDelegate;

class LeftView : public DTreeView
{
    Q_OBJECT
public:
    explicit LeftView(QWidget *parent = nullptr);
    QStandardItem *getNotepadRoot();
    QModelIndex setDefaultNotepadItem();
signals:
    void sigAction(QAction *action);
protected:
    void mousePressEvent(QMouseEvent *event);
private:
    void initDelegate();
    void initModel();
    void initNotepadRoot();
    void initMenu();
    DMenu               *m_notepadMenu {nullptr};
    QStandardItemModel  *m_pDataModel {nullptr};
    LeftViewDelegate    *m_pItemDelegate {nullptr};
    bool                 m_isEnable {true};
};

#endif // LEFTVIEW_H
