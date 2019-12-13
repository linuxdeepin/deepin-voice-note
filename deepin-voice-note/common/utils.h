#include <QString>
#include <QDateTime>
#include <QPixmap>

#include <DApplication>
#include <DTextEdit>

DWIDGET_USE_NAMESPACE

class Utils
{
public:
    Utils();
    static QString convertDateTime(const QDateTime &dateTime);
    static QPixmap renderSVG(const QString &filePath, const QSize &size, DApplication *pApp);
    static int highTextEdit(DTextEdit *textEdit,const QTextCharFormat *oriFormat,const QString &searchKey, const QColor &highColor);
};
