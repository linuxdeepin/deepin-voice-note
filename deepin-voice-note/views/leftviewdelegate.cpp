#include "leftviewdelegate.h"
#include "common/utils.h"

#include <QLineEdit>
#include <QTextItem>
#include <QDebug>

#include <DPalette>

//TODO:
// VFolderNamePHelper is used to highlight the folder
//name when search the notes
struct VFolderNamePHelper {
    VFolderNamePHelper(QPainter* painter, QFontMetrics fontMetrics, QRect nameRect)
        :m_fontMetrics(fontMetrics)
        , m_nameRect(nameRect)
        , m_painter(painter)
    {
        if (nullptr != m_painter) {
            m_pens[OldPen] = m_painter->pen();

            DPalette pl;
            QColor folderNamePenColor = pl.color(DPalette::Highlight);
            m_pens[FolderPen] = folderNamePenColor;
        }
    }

    struct Text {
        QString text;
        QRect   rect;
        bool    isKeyword {false};
    };

    enum {
        OldPen,
        FolderPen,
        PenCount
    };

    void spiltByKeyword(const QString &text, const QRegExp &keyword);
    void paintFolderName(bool isSelected = false);

    QVector<Text> m_textsVector;
    QFontMetrics m_fontMetrics;
    QPen         m_pens[PenCount];
    QRect        m_nameRect;
    QPainter*    m_painter {nullptr};
};

void VFolderNamePHelper::spiltByKeyword(const QString &text, const QRegExp &keyword)
{
    //Check if text exceed the name rect, elide the
    //text first
    QString elideText = m_fontMetrics.elidedText(text,Qt::ElideRight, m_nameRect.width());

    int keyLen = keyword.pattern().length();
    int textLen = text.length();
    int startPos = 0;
    int pos = 0;

    m_textsVector.clear();

    if (!keyword.isEmpty()) {
        while ((pos=elideText.indexOf(keyword,startPos)) != -1) {
            Text tb;

            if (startPos != pos) {
                int extraLen = pos-startPos;
                tb.text = elideText.mid(startPos, extraLen);
                startPos += extraLen;
                tb.rect = QRect(0, 0
                                ,m_fontMetrics.width(tb.text)
                                ,m_fontMetrics.height()
                                );
                m_textsVector.push_back(tb);

                tb.text = elideText.mid(pos, keyLen);
                tb.rect = QRect(0, 0
                                ,m_fontMetrics.width(tb.text)
                                ,m_fontMetrics.height()
                                );
                tb.isKeyword = true;
                m_textsVector.push_back(tb);
            } else {
                tb.text = elideText.mid(pos, keyLen);
                tb.rect = QRect(0, 0
                                ,m_fontMetrics.width(tb.text)
                                ,m_fontMetrics.height()
                                );
                tb.isKeyword = true;
                m_textsVector.push_back(tb);
            }

            startPos += keyLen;
        }
    }

    if (startPos < elideText.length()) {
        Text tb;

        tb.text = elideText.mid(startPos, (textLen-startPos));
        tb.rect = QRect(0, 0
                        ,m_fontMetrics.width(tb.text)
                        ,m_fontMetrics.height()
                        );

        m_textsVector.push_back(tb);
    }
}

void VFolderNamePHelper::paintFolderName(bool isSelected)
{
    int currentX = m_nameRect.x();
    int currentY = m_nameRect.y();

    for(auto it : m_textsVector) {
        int w = it.rect.width();
        int h = m_nameRect.height();

        it.rect.setX(currentX);
        it.rect.setY(currentY);
        it.rect.setSize(QSize(w, h));
        if (it.isKeyword) {
            //If the item is selected,don't need highlight keyword
            m_painter->setPen((isSelected ? m_pens[OldPen] : m_pens[FolderPen]));
        } else {
            m_painter->setPen(m_pens[OldPen]);
        }

        m_painter->drawText(it.rect, it.text);

        currentX += it.rect.width();
    }

    //Restore default pen
    m_painter->setPen(m_pens[OldPen]);
}

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

void LeftViewDelegate::updateSearchKeyword(const QRegExp &keyword)
{
    m_searchKeyword = keyword;
}

void LeftViewDelegate::resetKeyword()
{
    m_searchKeyword = QRegExp();
}

void LeftViewDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    if (index.isValid()) {
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
        if (index.row() == 0) {//第一行头上加5px
            paintRect.setY(rect.top() + 5);
        }

        QPainterPath path;
        const int radius = 8;

        bool isSelected = false;

        path.moveTo(paintRect.bottomRight() - QPoint(0, radius));
        path.lineTo(paintRect.topRight() + QPoint(0, radius));
        path.arcTo(QRect(QPoint(paintRect.topRight() - QPoint(radius * 2, 0)),QSize(radius * 2, radius * 2)), 0, 90);
        path.lineTo(paintRect.topLeft() + QPoint(radius, 0));
        path.arcTo(QRect(QPoint(paintRect.topLeft()), QSize(radius * 2, radius * 2)), 90, 90);
        path.lineTo(paintRect.bottomLeft() - QPoint(0, radius));
        path.arcTo(QRect(QPoint(paintRect.bottomLeft() - QPoint(0, radius * 2)), QSize(radius * 2, radius * 2)), 180, 90);
        path.lineTo(paintRect.bottomLeft() + QPoint(radius, 0));
        path.arcTo(QRect(QPoint(paintRect.bottomRight() - QPoint(radius * 2, radius * 2)), QSize(radius * 2, radius * 2)), 270, 90);

        if (option.state & QStyle::State_Selected) {
            QColor fillColor = option.palette.color(DPalette::Normal, DPalette::Highlight);
            painter->setBrush(QBrush(fillColor));
            painter->fillPath(path, painter->brush());
            painter->setPen(QPen(Qt::white));

            isSelected = true;
        } else if (option.state & QStyle::State_MouseOver) {
            painter->setBrush(QBrush(m_parentPb.color(DPalette::Light)));
            painter->fillPath(path, painter->brush());
            painter->setPen(QPen(m_parentPb.color(DPalette::Normal, DPalette::WindowText)));
        } else if (option.state & QStyle::State_Enabled) {
            painter->setBrush(QBrush(m_parentPb.color(DPalette::Normal,DPalette::ItemBackground)));
            painter->fillPath(path, painter->brush());
            painter->setPen(QPen(m_parentPb.color(DPalette::Normal, DPalette::WindowText)));
        }
        else { //disable
            painter->setBrush(QBrush(m_parentPb.color(DPalette::Disabled,DPalette::ItemBackground)));
            painter->fillPath(path, painter->brush());
        }

        QRect iconRect(paintRect.left() + 6, paintRect.top() + 12, 40, 40);
        painter->drawImage(iconRect, data->UI.icon);
        painter->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T6));
        QFontMetrics fontMetrics = painter->fontMetrics();
        int space = (paintRect.height() - fontMetrics.height() * 2)/2 + paintRect.top();
        QRect nameRect(paintRect.left() + 52, space, 170, fontMetrics.height());
        space += fontMetrics.height();
        QRect timeRect(paintRect.left() + 52, space, 170, fontMetrics.height());

        VFolderNamePHelper vfnphelper(painter, fontMetrics, nameRect);

        vfnphelper.spiltByKeyword(data->name, m_searchKeyword);
        vfnphelper.paintFolderName(isSelected);

        painter->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T8));
        painter->drawText(timeRect, Utils::convertDateTime(data->createTime));
        painter->restore();
    }
}
