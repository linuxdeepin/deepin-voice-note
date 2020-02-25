#ifndef FolderDelegate_H
#define FolderDelegate_H

#include <DStyledItemDelegate>
DWIDGET_USE_NAMESPACE

class FolderTree;

class FolderDelegate : public DStyledItemDelegate
{
public:
    enum ItemType {
        NOTEROOT, //记事本一级目录
        NOTEITEM, //记事本项
        ERRORTYPE
    };
    FolderDelegate(FolderTree *parent = nullptr);
public slots:
    void handleChangeTheme();

protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const Q_DECL_OVERRIDE;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const Q_DECL_OVERRIDE;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                              const QModelIndex &index) const override;
private:
    void init();
    void paintNoteRoot(QPainter *painter, const QStyleOptionViewItem &option,
                       const QModelIndex &index) const;
    void paintNoteItem(QPainter *painter, const QStyleOptionViewItem &option,
                       const QModelIndex &index) const;
    FolderTree *m_treeView {nullptr};
    DPalette m_parentPb;
};

#endif // FolderDelegate_H
