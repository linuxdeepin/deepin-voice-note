#include <QString>
#include <QDateTime>

class Utils
{
public:
    Utils();
    static QString convertDateTime(const QDateTime &dateTime);
};
