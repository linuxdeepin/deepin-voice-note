#include "folderdelegate.h"

#include <QPainter>
#include <QDebug>

#include <DApplication>
#include <DApplicationHelper>
#include <QLineEdit>

static QImage textImage;

FolderDelegate::FolderDelegate(QAbstractItemView *parent)
    : DStyledItemDelegate(parent)
    , m_parentView(parent)
{
    init();
    textImage = QImage(":/icons/deepin/builtin/default_folder_icons/1.svg");
}
void FolderDelegate::init()
{
    m_parentPb = DApplicationHelper::instance()->palette(m_parentView);
    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this,
            &FolderDelegate::handleChangeTheme);
    initNoteRoot();
}

void FolderDelegate::initNoteRoot()
{
    ;
}

QWidget *FolderDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                                      const QModelIndex &index) const
{
    QLineEdit *editBox = new QLineEdit(parent);
    editBox->setFixedSize(171, 30);
    return  editBox;
}

void FolderDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    return;
}

void FolderDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                  const QModelIndex &index) const
{
    return;
}

void FolderDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                                          const QModelIndex &index) const
{
    Q_UNUSED(index)
    QLineEdit *edit = static_cast<QLineEdit *>(editor);
    edit->move(option.rect.x() + 5, option.rect.y() + 13);
}

FolderDelegate::ItemType FolderDelegate::getItemType(const QModelIndex &index) const
{
    if (index.isValid()) {
        QVariant var = index.data(Qt::UserRole + 1);
        int value = var.toInt();
        if (value < ERRORTYPE) {
            return static_cast<FolderDelegate::ItemType>(value);
        }
    }
    return ERRORTYPE;
}

void FolderDelegate::handleChangeTheme()
{
    m_parentPb = DApplicationHelper::instance()->palette(m_parentView);
    m_parentView->update(m_parentView->currentIndex());
}

QSize FolderDelegate::sizeHint(const QStyleOptionViewItem &option,
                               const QModelIndex &index) const
{
    FolderDelegate::ItemType type = getItemType(index);
    switch (type) {
    case NOTEROOT:
        return  QSize(option.rect.width(), 1); //隐藏记事本一级目录
    case NOTEITEM:
        return  QSize(option.rect.width(), 47);
    default:
        return DStyledItemDelegate::sizeHint(option, index);
    }
}

void FolderDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                           const QModelIndex &index) const
{
    FolderDelegate::ItemType type = getItemType(index);
    switch (type) {
    case NOTEROOT:
        return  paintNoteRoot(painter, option, index);
    case NOTEITEM:
        return  paintNoteItem(painter, option, index);
    default:
        return DStyledItemDelegate::paint(painter, option, index);
    }
}

void FolderDelegate::paintNoteRoot(QPainter *painter, const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
{
    return;
}

void FolderDelegate::paintNoteItem(QPainter *painter, const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
{
    if(!index.isValid()){
        return;
    }
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHints(QPainter::SmoothPixmapTransform);
    QRect paintRect = option.rect;
    paintRect.setTop(paintRect.top() + 10); //item间隔
    paintRect.setRight(paintRect.right() - 10);
    QPainterPath path;
    const int radius = 8;
    path.moveTo(paintRect.bottomRight() - QPoint(0, radius));
    path.lineTo(paintRect.topRight() + QPoint(0, radius));
    path.arcTo(QRect(QPoint(paintRect.topRight() - QPoint(radius * 2, 0)), QSize(radius * 2, radius * 2)), 0, 90);
    path.lineTo(paintRect.topLeft() + QPoint(radius, 0));
    path.arcTo(QRect(QPoint(paintRect.topLeft()), QSize(radius * 2, radius * 2)), 90, 90);
    path.lineTo(paintRect.bottomLeft() - QPoint(0, radius));
    path.arcTo(QRect(QPoint(paintRect.bottomLeft() - QPoint(0, radius * 2)), QSize(radius * 2, radius * 2)), 180, 90);
    path.lineTo(paintRect.bottomLeft() + QPoint(radius, 0));
    path.arcTo(QRect(QPoint(paintRect.bottomRight() - QPoint(radius * 2, radius * 2)), QSize(radius * 2, radius * 2)), 270, 90);

    bool enable = option.state & QStyle::State_Enabled;
    if (option.state & QStyle::State_Selected) {
        QColor fillColor;
        if (enable) {
            fillColor = option.palette.color(DPalette::Normal, DPalette::Highlight);
        } else {
            fillColor = option.palette.color(DPalette::Disabled, DPalette::Highlight);
        }
        painter->setBrush(QBrush(fillColor));
        painter->fillPath(path, painter->brush());
        painter->setPen(QPen(Qt::white));
    } else {
        if (enable == false) {
            painter->setBrush(QBrush(m_parentPb.color(DPalette::Disabled, DPalette::ItemBackground)));
            painter->fillPath(path, painter->brush());
            painter->setPen(QPen(m_parentPb.color(DPalette::Disabled, DPalette::WindowText)));
        } else {
            if (option.state & QStyle::State_MouseOver) {
                painter->setBrush(QBrush(m_parentPb.color(DPalette::Light)));
                painter->fillPath(path, painter->brush());
                painter->setPen(QPen(m_parentPb.color(DPalette::Normal, DPalette::WindowText)));
            } else {
                painter->setBrush(QBrush(m_parentPb.color(DPalette::Normal, DPalette::ItemBackground)));
                painter->fillPath(path, painter->brush());
                painter->setPen(QPen(m_parentPb.color(DPalette::Normal, DPalette::WindowText)));
            }
        }
    }
    QFontMetrics fontMetrics = painter->fontMetrics();
    QString strNum = QString::number(20);
    int numWidth = fontMetrics.width(strNum);
    int iconSpace = (paintRect.height() - 24) / 2;
    QRect iconRect(paintRect.left() + 11, paintRect.top() + iconSpace, 24, 24);
    QRect numRect(paintRect.right() - numWidth - 7, paintRect.top(), numWidth, paintRect.height());
    QRect nameRect(iconRect.right() + 12, paintRect.top(), numRect.left() - iconRect.right() - 12, paintRect.height());
    painter->drawText(numRect, Qt::AlignRight | Qt::AlignVCenter, strNum);
    painter->drawImage(iconRect, textImage);
    painter->drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, QString("记事本１"));
    painter->restore();
}


