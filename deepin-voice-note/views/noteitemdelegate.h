#ifndef NOTEITEMDELEGATE_H
#define NOTEITEMDELEGATE_H

#include <DStyledItemDelegate>

DWIDGET_USE_NAMESPACE

class NoteListView;

class NoteItemDelegate : public DStyledItemDelegate
{
    Q_OBJECT
public:
    NoteItemDelegate(NoteListView *parent = nullptr);
    void setSearchKey(const QRegExp &key);
    void handleChangeTheme();

protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const Q_DECL_OVERRIDE;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,  const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                              const QModelIndex &index) const override;
private:
    void paintItemBase(QPainter *painter,const QStyleOptionViewItem &option,
                       const QRect &rect,bool &isSelect) const;
    void paintNormalItem(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const;
    void paintSearchItem(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const;

    NoteListView *m_parentView {nullptr};
    QRegExp m_searchKey;
    DPalette m_parentPb;
};

#endif // NOTEITEMDELEGATE_H
