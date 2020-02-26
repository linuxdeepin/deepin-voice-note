#include "folderdelegate.h"
#include "foldertreecommon.h"
#include "common/vnoteforlder.h"
#include "db/vnotefolderoper.h"

#include <QPainter>
#include <QLineEdit>

#include <DApplication>
#include <DApplicationHelper>

FolderDelegate::FolderDelegate(QAbstractItemView *parent)
    : DStyledItemDelegate(parent)
    , m_treeView(parent)
{
    init();
}
void FolderDelegate::init()
{
    m_parentPb = DApplicationHelper::instance()->palette(m_treeView);
    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this,
            &FolderDelegate::handleChangeTheme);
}

QWidget *FolderDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                                      const QModelIndex &index) const
{
    Q_UNUSED(index)
    Q_UNUSED(option)
    QLineEdit *editBox = new QLineEdit(parent);
    editBox->setFixedSize(171, 30);
    return  editBox;
}

void FolderDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QVariant var = index.data(Qt::UserRole + 2);
    VNoteFolder *data = static_cast<VNoteFolder *>(var.value<void *>());
    QLineEdit *edit = static_cast<QLineEdit *>(editor);
    edit->setText(data->name);
}

void FolderDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                  const QModelIndex &index) const
{
    Q_UNUSED(model);
    QLineEdit *edit = static_cast<QLineEdit *>(editor);
    QString text = edit->displayText();
    if (!text.isEmpty()) {
        if (FolderTreeCommon::getStandardItemType(index) == FolderTreeCommon::NOTEPADITEM) {
            VNoteFolder *folderdata = static_cast<VNoteFolder *>(FolderTreeCommon::getStandardItemData(index));
            if (folderdata && folderdata->name != text) {
                VNoteFolderOper folderOper(folderdata);
                folderOper.renameVNoteFolder(text);
            }
        }
    }
}

void FolderDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                                          const QModelIndex &index) const
{
    Q_UNUSED(index)
    QLineEdit *edit = static_cast<QLineEdit *>(editor);
    edit->move(option.rect.x() + 15, option.rect.y() + 8);
}

void FolderDelegate::handleChangeTheme()
{
    m_parentPb = DApplicationHelper::instance()->palette(m_treeView);
    m_treeView->update(m_treeView->currentIndex());
}

QSize FolderDelegate::sizeHint(const QStyleOptionViewItem &option,
                               const QModelIndex &index) const
{
    FolderTreeCommon::StandardItemType type = FolderTreeCommon::getStandardItemType(index);
    switch (type) {
    case FolderTreeCommon::NOTEPADROOT:
        return  QSize(option.rect.width(), 1); //隐藏记事本一级目录
    case FolderTreeCommon::NOTEPADITEM:
        return  QSize(option.rect.width(), 47);
    default:
        return DStyledItemDelegate::sizeHint(option, index);
    }
}

void FolderDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                           const QModelIndex &index) const
{
    FolderTreeCommon::StandardItemType type = FolderTreeCommon::getStandardItemType(index);
    switch (type) {
    case FolderTreeCommon::NOTEPADROOT:
        return  paintNoteRoot(painter, option, index);
    case FolderTreeCommon::NOTEPADITEM:
        return  paintNoteItem(painter, option, index);
    default:
        return DStyledItemDelegate::paint(painter, option, index);
    }
}

void FolderDelegate::paintNoteRoot(QPainter *painter, const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
{
    Q_UNUSED(painter)
    Q_UNUSED(index)
    Q_UNUSED(option)
}

void FolderDelegate::paintNoteItem(QPainter *painter, const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
{
    if (!index.isValid()) {
        return;
    }
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHints(QPainter::SmoothPixmapTransform);
    QRect paintRect(option.rect.x() + 10, option.rect.y() + 5,
                    option.rect.width() - 20, option.rect.height() - 10);
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
    VNoteFolder *data = static_cast<VNoteFolder *>(FolderTreeCommon::getStandardItemData(index));
    if (data != nullptr) {
        QFontMetrics fontMetrics = painter->fontMetrics();
        QString strNum = QString::number(data->notesCount);
        int numWidth = fontMetrics.width(strNum);
        int iconSpace = (paintRect.height() - 24) / 2;
        QRect iconRect(paintRect.left() + 11, paintRect.top() + iconSpace, 24, 24);
        QRect numRect(paintRect.right() - numWidth - 7, paintRect.top(), numWidth, paintRect.height());
        QRect nameRect(iconRect.right() + 12, paintRect.top(),
                       numRect.left() - iconRect.right() - 15, paintRect.height());
        painter->drawText(numRect, Qt::AlignRight | Qt::AlignVCenter, strNum);
        painter->drawImage(iconRect, data->UI.icon);
        QString elideText = fontMetrics.elidedText(data->name, Qt::ElideRight, nameRect.width());
        painter->drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, elideText);
    }
    painter->restore();
}


