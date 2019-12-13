#include "leftviewdelegate.h"
#include "common/utils.h"

#include<QLineEdit>

LeftViewDelegate::LeftViewDelegate(QAbstractItemView *parent)
    : DStyledItemDelegate(parent)
    , m_parentView(parent)
{
    m_parentPb = DApplicationHelper::instance()->palette(m_parentView);
    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this,
            &LeftViewDelegate::handleChangeTheme);
}

QSize LeftViewDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index)
    return QSize(option.rect.width(), 69);
}

QWidget *LeftViewDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                                        const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    QLineEdit *editBox = new QLineEdit(parent);
    DPalette pe = DApplicationHelper::instance()->palette(m_parentView);
    pe.setBrush(DPalette::Button, pe.color(DPalette::Inactive, DPalette::Highlight));
    pe.setBrush(DPalette::Text, Qt::white);
    pe.setBrush(DPalette::Base, pe.color(DPalette::Normal, DPalette::Highlight));
    editBox->autoFillBackground();

    editBox->setFixedSize(170, 45);
    editBox->setPalette(pe);
    return editBox;
}

void LeftViewDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QVariant var = index.data(Qt::UserRole + 1);
    VNoteFolder *data = static_cast<VNoteFolder *>(var.value<void *>());
    QLineEdit *edit = static_cast<QLineEdit *>(editor);
    edit->setText(data->name);
}

void LeftViewDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                    const QModelIndex &index) const
{
    Q_UNUSED(model)
    QLineEdit *edit = static_cast<QLineEdit *>(editor);
    QVariant var = index.data(Qt::UserRole + 1);
    VNoteFolder *data = static_cast<VNoteFolder *>(var.value<void *>());
    QString text = edit->displayText();
    if (!text.isEmpty() && text != data->name)
    {
        data->name = text;
        emit sigFolderRename(data);
    }
}

void LeftViewDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                                            const QModelIndex &index) const
{
    Q_UNUSED(index)
    QLineEdit *edit = static_cast<QLineEdit *>(editor);
    edit->move(option.rect.x() + 55, option.rect.y() + 12);
}

void LeftViewDelegate::handleChangeTheme()
{
    m_parentPb = DApplicationHelper::instance()->palette(m_parentView);
    m_parentView->update(m_parentView->currentIndex());
}

void LeftViewDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    if (index.isValid())
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        QVariant var = index.data(Qt::UserRole + 1);
        VNoteFolder *data = static_cast<VNoteFolder *>(var.value<void *>());
        QStyleOptionViewItem viewOption(option);

        QRect rect;
        rect.setX(option.rect.x());
        rect.setY(option.rect.y());
        rect.setWidth(option.rect.width());
        rect.setHeight(option.rect.height());

        painter->setRenderHints(QPainter::SmoothPixmapTransform);
        QRect paintRect = QRect(rect.left() + 5, rect.top(), rect.width() - 10, rect.height());
        if (index.row() == 0)  //第一行头上加5px
        {
            paintRect.setY(rect.top() + 5);
        }
        QPainterPath path;
        const int radius = 8;

        path.moveTo(paintRect.bottomRight() - QPoint(0, radius));
        path.lineTo(paintRect.topRight() + QPoint(0, radius));
        path.arcTo(QRect(QPoint(paintRect.topRight() - QPoint(radius * 2, 0)),QSize(radius * 2, radius * 2)), 0, 90);
        path.lineTo(paintRect.topLeft() + QPoint(radius, 0));
        path.arcTo(QRect(QPoint(paintRect.topLeft()), QSize(radius * 2, radius * 2)), 90, 90);
        path.lineTo(paintRect.bottomLeft() - QPoint(0, radius));
        path.arcTo(QRect(QPoint(paintRect.bottomLeft() - QPoint(0, radius * 2)), QSize(radius * 2, radius * 2)), 180, 90);
        path.lineTo(paintRect.bottomLeft() + QPoint(radius, 0));
        path.arcTo(QRect(QPoint(paintRect.bottomRight() - QPoint(radius * 2, radius * 2)), QSize(radius * 2, radius * 2)), 270, 90);
        if (option.state & QStyle::State_Selected)
        {
            QColor fillColor = option.palette.color(DPalette::Normal, DPalette::Highlight);
            painter->setBrush(QBrush(fillColor));
            painter->fillPath(path, painter->brush());
            painter->setPen(QPen(Qt::white));
        }
        else if (option.state & QStyle::State_MouseOver)
        {
            painter->setBrush(QBrush(m_parentPb.color(DPalette::Light)));
            painter->fillPath(path, painter->brush());
        }
        else if (option.state & QStyle::State_Enabled)
        {
            painter->setBrush(QBrush(m_parentPb.color(DPalette::ItemBackground)));
            painter->fillPath(path, painter->brush());
            painter->setPen(QPen(m_parentPb.color(DPalette::Normal, DPalette::WindowText)));
        }
        QRect iconRect(paintRect.left() + 6, paintRect.top() + 12, 40, 40);
        QRect nameRect(paintRect.left() + 52, paintRect.top() + 12, 160, 20);
        QRect timeRect(paintRect.left() + 52, paintRect.top() + 34, 160, 18);

        painter->drawImage(iconRect, data->UI.icon);
        painter->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T6));
        painter->drawText(nameRect, data->name);
        painter->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T8));
        painter->drawText(timeRect, Utils::convertDateTime(data->createTime));
        painter->restore();
    }
}
