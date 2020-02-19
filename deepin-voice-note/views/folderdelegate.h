#ifndef FolderDelegate_H
#define FolderDelegate_H
#include <QImage>

#include <DStyledItemDelegate>
DWIDGET_USE_NAMESPACE

class FolderDelegate : public DStyledItemDelegate
{
public:
    enum ItemType {
        NOTEROOT, //记事本一级目录
        NOTEITEM, //记事本项
        ERRORTYPE
    };
    FolderDelegate(QAbstractItemView *parent = nullptr);
    ItemType getItemType(const QModelIndex &index) const;
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
    void initNoteRoot();
    void paintNoteRoot(QPainter *painter, const QStyleOptionViewItem &option,
                       const QModelIndex &index) const;
    void paintNoteItem(QPainter *painter, const QStyleOptionViewItem &option,
                       const QModelIndex &index) const;
    QAbstractItemView *m_parentView {nullptr};
    DPalette m_parentPb;
};

#endif // FolderDelegate_H
