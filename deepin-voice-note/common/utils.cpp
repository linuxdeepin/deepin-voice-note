#include "utils.h"
#include <QImageReader>
Utils::Utils() {}

QString Utils::convertDateTime(const QDateTime &dateTime)
{
    QDate currDateTime = QDate::currentDate();
    QDate folerDate = dateTime.date();
    QString disptime;
    qint64 offset = folerDate.daysTo(currDateTime);
    if (0 == offset) {
        //今天：展示时间hh：mm ；
        disptime = dateTime.toString("hh:mm");
    } else if (1 == offset) {
        //昨天
        disptime.append("昨天");
    } else if (folerDate.year() == currDateTime.year()) {
        //不跨年 其他时间：MM-DD（例如：9⽉27日）；
        disptime = dateTime.toString("MM月dd日");
    } else {
        //跨年:YYYY-MM-DD（例如2018年9月26日）；
        disptime = dateTime.toString("yyyy年MM月dd日");
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

int Utils::highTextEdit(DTextEdit *textEdit, const QTextCharFormat &oriFormat,const QRegExp &searchKey,
                         const QColor &highColor)
{
    int findCount = 0;
    QTextCursor find_cursor = textEdit->textCursor();
    QTextCharFormat colorFormat = oriFormat;
    colorFormat.setForeground(highColor);
    while (textEdit->find(searchKey)) {
        find_cursor.movePosition(QTextCursor::WordRight,QTextCursor::KeepAnchor);
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

QString Utils::formatMillisecond(qint64 millisecond)
{
    uint minSecond = 1;
    uint curSecond = static_cast<uint>(millisecond / 1000);
    if(curSecond < minSecond){
        curSecond = 1;
    }
    if(curSecond < 3600){
        return QDateTime::fromTime_t(curSecond).toUTC().toString("mm:ss");
    }
    return QDateTime::fromTime_t(curSecond).toUTC().toString("hh:mm:ss");
}
