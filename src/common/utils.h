#include <QString>
#include <QDateTime>
#include <QPixmap>
#include <QTextDocumentFragment>
#include <QTextDocument>

#include <DApplication>
#include <DTextEdit>

DWIDGET_USE_NAMESPACE

class VNoteBlock;

class Utils
{
public:
    Utils();
    static QString convertDateTime(const QDateTime &dateTime);
    static QPixmap renderSVG(const QString &filePath, const QSize &size, DApplication *pApp);
    static QPixmap loadSVG(const QString &fileName, bool fCommon = false);
    static int highTextEdit(QTextDocument *textDoc,const QString &searchKey,
                            const QColor &highColor, bool undo = false);
    static QString formatMillisecond(qint64 millisecond, bool minValue = 1);

    static void documentToBlock(VNoteBlock *block, const QTextDocument *doc);
    static void blockToDocument(const VNoteBlock *block, QTextDocument *doc);
    static void setDefaultColor(QTextDocument *srcDoc, const QColor &color);
};
