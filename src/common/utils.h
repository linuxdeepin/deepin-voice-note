/*
* Copyright (C) 2019 ~ 2019 UnionTech Software Technology Co.,Ltd.
*
* Author:     V4fr3e <V4fr3e@deepin.io>
*
* Maintainer: V4fr3e <liujinli@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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
