#include "utils.h"
#include "globaldef.h"
#include "vnoteitem.h"

#include <QImageReader>
#include <QIcon>
#include <QDebug>
#include <QTextBlock>

#include <DGuiApplicationHelper>

Utils::Utils() {}

QString Utils::convertDateTime(const QDateTime &dateTime)
{
    QDate currDateTime = QDate::currentDate();
    QDate folerDate = dateTime.date();
    QString disptime;
    qint64 offset = folerDate.daysTo(currDateTime);
    if (0 == offset) {
        qint64 offsetSec = dateTime.secsTo(QDateTime::currentDateTime());
        if (offsetSec < 3600) {
            offsetSec /= 60;
            if(offsetSec <= 1){
                disptime = DApplication::translate("Utils","1 min ago");
            }else {
                disptime = DApplication::translate("Utils","%1 mins ago").arg(offsetSec);
            }
        } else {
            disptime = dateTime.toString("hh:mm");
        }
    } else if (1 == offset) {
        disptime = DApplication::translate("Utils","Yesterday") + " " + dateTime.toString("hh:mm");
    } else if (folerDate.year() == currDateTime.year()) {
        //不跨年 其他时间：MM-DD（例如：9-27）；
        disptime = dateTime.toString("MM-dd");
    } else {
        //跨年:YYYY-MM-DD（例如2018-9-26）；
        disptime = dateTime.toString("yyyy-MM-dd");
    }

    return disptime;
}

QPixmap Utils::renderSVG(const QString &filePath, const QSize &size, DApplication *pApp)
{
    QImageReader reader;
    QPixmap pixmap;

    reader.setFileName(filePath);

    if (reader.canRead()) {
        // const qreal ratio = qApp->devicePixelRatio();
        const qreal ratio = pApp->devicePixelRatio();
        reader.setScaledSize(size * ratio);
        pixmap = QPixmap::fromImage(reader.read());
        pixmap.setDevicePixelRatio(ratio);
    } else {
        pixmap.load(filePath);
    }

    return pixmap;
}

QPixmap Utils::loadSVG(const QString &fileName, bool fCommon)
{
    DGuiApplicationHelper::ColorType theme =
        DGuiApplicationHelper::instance()->themeType();

    QString iconPath(STAND_ICON_PAHT);

    if (!fCommon) {
        if (DGuiApplicationHelper::ColorType::LightType == theme) {
            iconPath += QString("light/");
        } else {
            iconPath += QString("dark/");
        }
    }

    iconPath += fileName;

    qreal ratio = 1.0;

    const qreal devicePixelRatio = qApp->devicePixelRatio();

    QPixmap pixmap;

    if (!qFuzzyCompare(ratio, devicePixelRatio)) {
        QImageReader reader;
        reader.setFileName(qt_findAtNxFile(iconPath, devicePixelRatio, &ratio));
        if (reader.canRead()) {
            reader.setScaledSize(reader.size() * (devicePixelRatio / ratio));
            pixmap = QPixmap::fromImage(reader.read());
            pixmap.setDevicePixelRatio(devicePixelRatio);
        }
    } else {
        pixmap.load(iconPath);
    }

    return pixmap;
}

int Utils::highTextEdit(DTextEdit *textEdit, const QTextCharFormat &oriFormat, const QString &searchKey,
                        const QColor &highColor)
{
    int findCount = 0;
    QTextCursor find_cursor = textEdit->textCursor();
    QTextCharFormat colorFormat = oriFormat;
    colorFormat.setForeground(highColor);
    while (textEdit->find(searchKey)) {
        find_cursor.movePosition(QTextCursor::WordRight, QTextCursor::KeepAnchor);
        textEdit->mergeCurrentCharFormat(colorFormat);
        findCount ++;
    }
    if (findCount) {
        find_cursor.clearSelection();
        find_cursor.movePosition(QTextCursor::EndOfWord);
        textEdit->setTextCursor(find_cursor);
        textEdit->setCurrentCharFormat(oriFormat);
    }
    return  findCount;
}

QString Utils::formatMillisecond(qint64 millisecond, bool minValue)
{
    uint curSecond = static_cast<uint>(millisecond / 1000);
    if (curSecond < minValue) {
        curSecond = minValue;
    }
    if (curSecond < 3600) {
        return QDateTime::fromTime_t(curSecond).toUTC().toString("mm:ss");
    }

    return QString("60:00");
}

void Utils::documentToBlock(VNoteBlock *block, const QTextDocument *doc)
{
    if(block != nullptr){
       block->blockText = "";
    }

    if(doc != nullptr){
        QTextBlock currentBlock = doc->begin();
        QTextBlock::iterator it;
        while(true){
            for (it = currentBlock.begin(); !(it.atEnd()); ){
                QTextFragment currentFragment = it.fragment();
//                QTextImageFormat newImageFormat = currentFragment.charFormat().toImageFormat();
//                if (newImageFormat.isValid()) {
//                    int pos = currentFragment.position();
//                    qDebug() << "image block:" << pos <<"url;" << newImageFormat.name();
//                    ++it;
//                    continue;
//                }
                if (currentFragment.isValid()){
                    ++it;
                    block->blockText.append(currentFragment.text());
                }

            }
            currentBlock = currentBlock.next();
            if(!currentBlock.isValid())
                break;
        }
    }
}

void Utils::blockToDocument(const VNoteBlock *block, QTextDocument *doc)
{
    if(block && doc){
        doc->setPlainText(block->blockText);
    }
}
