#include "utils.h"

Utils::Utils()
{

}

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


