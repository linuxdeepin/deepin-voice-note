#ifndef LEFTVIEWDELEGATE_H
#define LEFTVIEWDELEGATE_H

#include <DStyledItemDelegate>
DWIDGET_USE_NAMESPACE

class MiddleViewDelegate : public DStyledItemDelegate
{
    Q_OBJECT
public:
    MiddleViewDelegate(QAbstractItemView *parent = nullptr);
    void handleChangeTheme();
    void setSearchKey(const QRegExp &key);
    void setEnableItem(bool enable);
    const int MAX_TITLE_LEN = 64;
protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,  const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                              const QModelIndex &index) const override;
private:
    void paintItemBase(QPainter *painter, const QStyleOptionViewItem &option,
                       const QRect &rect, bool &isSelect) const;
    void paintNormalItem(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const;
    void paintSearchItem(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const;

    QAbstractItemView *m_parentView {nullptr};
    QRegExp m_searchKey;
    DPalette m_parentPb;
    bool     m_enableItem {true};
};

#endif // LEFTVIEWDELEGATE_H
