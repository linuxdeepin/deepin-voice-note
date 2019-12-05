#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QDateTime>

class Utils
{
public:
    Utils();
    static QString convertDateTime(const QDateTime &dateTime);
};

#endif // UTILS_H
