#ifndef MIDDLEVIEW_H
#define MIDDLEVIEW_H

#include <DWidget>
#include <DLabel>
#include <DStackedWidget>

DWIDGET_USE_NAMESPACE
class VNoteIconButton;
class NoteListView;
struct VNoteFolder;
struct VNoteItem;

class MiddleView : public DWidget
{
    Q_OBJECT
public:
    MiddleView(QWidget *parent = nullptr);
    void initNoteData(VNoteFolder * notepad,const QRegExp& searchKey);
    void        clearNoteData();
    VNoteItem*  getNoteData(const QModelIndex &index) const;
signals:
    void sigNotepadItemChange(const QModelIndex &index);
public slots:
    void onAddNote();
    void onRenameItem();
    void onDeleteItem();
    void onNoteChange(const QModelIndex &previous);
protected:
    void mousePressEvent(QMouseEvent *event);
private:
    void initUi();
    void initConnection();
    NoteListView *m_noteListView {nullptr};
    VNoteIconButton *m_addNoteBtn{nullptr};
    VNoteFolder *m_notepad {nullptr};
    DLabel *m_noSearchResultLab {nullptr};
    DStackedWidget *m_centerWidget {nullptr};
};

#endif // MIDDLEVIEW_H
