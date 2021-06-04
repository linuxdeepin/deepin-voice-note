/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
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
#include <DApplication>
#include <DTextEdit>

#include <QString>
#include <QDateTime>
#include <QPixmap>
#include <QTextDocumentFragment>
#include <QTextDocument>

DWIDGET_USE_NAMESPACE

class VNoteBlock;

class Utils
{
public:
    Utils();
    //格式化时间
    static QString convertDateTime(const QDateTime &dateTime);
    //加载图标
    static QPixmap renderSVG(const QString &filePath, const QSize &size, DApplication *pApp);
    //加载图标
    static QPixmap loadSVG(const QString &fileName, bool fCommon = false);
    //编辑器内容高亮
    static int highTextEdit(QTextDocument *textDoc, const QString &searchKey,
                            const QColor &highColor, bool undo = false);
    //格式化录音时长
    static QString formatMillisecond(qint64 millisecond, qint64 minValue = 1);
    //内存数据同步编辑器显示内容
    static void documentToBlock(VNoteBlock *block, const QTextDocument *doc);
    //编辑器同步内存数据
    static void blockToDocument(const VNoteBlock *block, QTextDocument *doc);
    //设置编辑器字体颜色
    static void setDefaultColor(QTextDocument *srcDoc, const QColor &color);
    //设置标题栏tab焦点
    static void setTitleBarTabFocus(QKeyEvent *event);
};
