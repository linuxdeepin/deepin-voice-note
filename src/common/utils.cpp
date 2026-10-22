// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "utils.h"
#include "ddlog.h"
#include "globaldef.h"
#include "vnoteitem.h"
// #include "vnoteapplication.h"

#include <DGuiApplicationHelper>
#include <DLog>

#include <QImageReader>
#include <QIcon>
#include <QDebug>
#include <QTextBlock>
#include <QBuffer>
#include <QFileInfo>
#include <QProcess>
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
#include <QRegularExpression>
#else
#include <QRegExp>
#endif
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QDBusInterface>
#include <QDBusConnection>
#include <QDir>
#include <QStandardPaths>

Utils::Utils()
{
    qCInfo(voiceNoteLog) << "Utils constructor called";
}

QString Utils::makeVoiceAbsolute(const QString &voicePath)
{
    const QUrl url(voicePath);
    if (url.isValid() && !url.scheme().isEmpty())
        return voicePath;
    const QString vNative = QDir::toNativeSeparators(voicePath);
    if (QDir::isAbsolutePath(vNative))
        return vNative;
    const QString appData = QDir::toNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    if (appData.isEmpty())
        return voicePath;
    return QDir::toNativeSeparators(QDir::cleanPath(appData + QDir::separator() + vNative));
}

/**
 * @brief Utils::convertDateTime
 * @param dateTime 时间
 * @return 格式化后的字符串
 */
QString Utils::convertDateTime(const QDateTime &dateTime)
{
    qCInfo(voiceNoteLog) << "Converting date time:" << dateTime.toString();
    QDate currDateTime = QDate::currentDate();
    QDate folerDate = dateTime.date();
    QString disptime;
    qint64 offset = folerDate.daysTo(currDateTime);
    
    qCDebug(voiceNoteLog) << "Converting date time:" << dateTime.toString() << "offset days:" << offset;
    
    if (0 == offset) {
        qCInfo(voiceNoteLog) << "Offset is 0";
        qint64 offsetSec = dateTime.secsTo(QDateTime::currentDateTime());
        if (offsetSec < 3600) {
            offsetSec /= 60;
            if (offsetSec <= 1) {
                disptime = DApplication::translate("Utils", "1 min ago");
            } else {
                qCInfo(voiceNoteLog) << "Offset is greater than 1 minute";
                disptime = DApplication::translate("Utils", "%1 mins ago").arg(offsetSec);
            }
            qCDebug(voiceNoteLog) << "Time is" << offsetSec << "minutes ago";
        } else {
            disptime = dateTime.toString("hh:mm");
            qCDebug(voiceNoteLog) << "Time is today at" << disptime;
        }
    } else if (1 == offset) {
        disptime = DApplication::translate("Utils", "Yesterday") + " " + dateTime.toString("hh:mm");
        qCDebug(voiceNoteLog) << "Time is yesterday at" << dateTime.toString("hh:mm");
    } else if (folerDate.year() == currDateTime.year()) {
        //不跨年 其他时间：MM-DD（例如：9-27）；
        disptime = dateTime.toString("MM-dd");
        qCDebug(voiceNoteLog) << "Time is this year on" << disptime;
    } else {
        //跨年:YYYY-MM-DD（例如2018-9-26）；
        disptime = dateTime.toString("yyyy-MM-dd");
        qCDebug(voiceNoteLog) << "Time is on" << disptime;
    }

    return disptime;
}

/**
 * @brief Utils::renderSVG
 * @param filePath
 * @param size
 * @param pApp
 * @return 加载的图标
 */
QPixmap Utils::renderSVG(const QString &filePath, const QSize &size, DApplication *pApp)
{
    qCDebug(voiceNoteLog) << "Rendering SVG:" << filePath << "with size:" << size;
    
    QImageReader reader;
    QPixmap pixmap;

    reader.setFileName(filePath);

    if (reader.canRead()) {
        qCInfo(voiceNoteLog) << "SVG can read";
        // const qreal ratio = qApp->devicePixelRatio();
        const qreal ratio = pApp->devicePixelRatio();
        reader.setScaledSize(size * ratio);
        pixmap = QPixmap::fromImage(reader.read());
        pixmap.setDevicePixelRatio(ratio);
        qCDebug(voiceNoteLog) << "SVG rendered successfully with ratio:" << ratio;
    } else {
        qCWarning(voiceNoteLog) << "Failed to read SVG file:" << filePath;
        pixmap.load(filePath);
    }

    return pixmap;
}

/**
 * @brief Utils::loadSVG
 * @param fileName
 * @param fCommon 图标是否区分主题，true 不区分
 * @return 加载的图标
 */
QPixmap Utils::loadSVG(const QString &fileName, bool fCommon)
{
    qCDebug(voiceNoteLog) << "Loading SVG:" << fileName << "common:" << fCommon;
    
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
    qCDebug(voiceNoteLog) << "Full icon path:" << iconPath;

    qreal ratio = 1.0;

    const qreal devicePixelRatio = qGuiApp->devicePixelRatio();

    QPixmap pixmap;

    if (!qFuzzyCompare(ratio, devicePixelRatio)) {
        qCInfo(voiceNoteLog) << "Device pixel ratio is not the same";
        QImageReader reader;
        reader.setFileName(qt_findAtNxFile(iconPath, devicePixelRatio, &ratio));
        if (reader.canRead()) {
            qCInfo(voiceNoteLog) << "SVG can read";
            reader.setScaledSize(reader.size() * (devicePixelRatio / ratio));
            pixmap = QPixmap::fromImage(reader.read());
            pixmap.setDevicePixelRatio(devicePixelRatio);
            qCDebug(voiceNoteLog) << "SVG loaded with device pixel ratio:" << devicePixelRatio;
        } else {
            qCWarning(voiceNoteLog) << "Failed to read SVG file:" << iconPath;
        }
    } else {
        qCInfo(voiceNoteLog) << "Device pixel ratio is the same";
        pixmap.load(iconPath);
        qCDebug(voiceNoteLog) << "SVG loaded with default ratio";
    }

    return pixmap;
}

/**
 * @brief Utils::highTextEdit
 * @param textDoc 编辑器的文档
 * @param searchKey 搜索关键字
 * @param highColor 高亮颜色
 * @param undo 是否取消文档上一步操作，true 取消
 * @return 搜索结果个数
 */
int Utils::highTextEdit(QTextDocument *textDoc, const QString &searchKey,
                        const QColor &highColor, bool undo)
{
    qCDebug(voiceNoteLog) << "Highlighting text with key:" << searchKey << "color:" << highColor.name() << "undo:" << undo;
    
    int findCount = 0;
    int len = searchKey.length();
    if (undo == true) {
        qCInfo(voiceNoteLog) << "Undoing previous operation";
        textDoc->undo();
    }
    if (len != 0) {
        qCInfo(voiceNoteLog) << "Searching for text";
        QTextCursor highlightCursor(textDoc);
        QTextCursor cursor(textDoc);

        cursor.beginEditBlock();
        QTextCharFormat plainFormat(highlightCursor.charFormat());
        QTextCharFormat colorFormat = plainFormat;
        colorFormat.setForeground(highColor);

        while (!highlightCursor.isNull() && !highlightCursor.atEnd()) {
            highlightCursor = textDoc->find(searchKey, highlightCursor,
                                            QTextDocument::FindFlags());

            if (!highlightCursor.isNull()) {
                int pos = highlightCursor.position();
                highlightCursor.setPosition(pos - len);
                highlightCursor.setPosition(pos, QTextCursor::KeepAnchor);
                highlightCursor.mergeCharFormat(colorFormat);
                findCount++;
            }
        }
        cursor.endEditBlock();
        qCDebug(voiceNoteLog) << "Found" << findCount << "matches";
    }

    return findCount;
}

/**
 * @brief Utils::formatMillisecond
 * @param millisecond 毫秒
 * @param minValue 最小显示的时间
 * @return 格式化后的字符串
 */
QString Utils::formatMillisecond(qint64 millisecond, qint64 minValue)
{
    qCDebug(voiceNoteLog) << "Formatting millisecond:" << millisecond << "min value:" << minValue;
    
    qint64 curSecond = millisecond / 1000;
    if (curSecond < minValue) {
        curSecond = minValue;
        qCDebug(voiceNoteLog) << "Adjusted to minimum value:" << minValue;
    }
    if (curSecond < 3600) {
        return QDateTime::fromSecsSinceEpoch(static_cast<qint64>(curSecond)).toUTC().toString("hh:mm:ss");
    }

    qCDebug(voiceNoteLog) << "Time exceeds 1 hour, returning maximum value";
    return QString("60:00:00");
}

/**
 * @brief Utils::documentToBlock
 * @param block 绑定的数据
 * @param doc 编辑器的文档
 */
void Utils::documentToBlock(VNoteBlock *block, const QTextDocument *doc)
{
    qCDebug(voiceNoteLog) << "Converting document to block";
    
    if (block != nullptr) {
        qCInfo(voiceNoteLog) << "Block is not nullptr";
        block->blockText = "";
    }

    if (doc != nullptr) {
        qCInfo(voiceNoteLog) << "Doc is not nullptr";
        QTextBlock currentBlock = doc->begin();
        QTextBlock::iterator it;
        while (true) {
            for (it = currentBlock.begin(); !(it.atEnd());) {
                QTextFragment currentFragment = it.fragment();
                               QTextImageFormat newImageFormat = currentFragment.charFormat().toImageFormat();
                               if (newImageFormat.isValid()) {
                                   int pos = currentFragment.position();
                                   qCDebug(voiceNoteLog) << "Found image at position:" << pos << "url:" << newImageFormat.name();
                                   ++it;
                                   continue;
                               }
                if (currentFragment.isValid()) {
                    ++it;
                    block->blockText.append(currentFragment.text());
                }
            }

            currentBlock = currentBlock.next();
            if (!currentBlock.isValid())
                break;

            block->blockText.append('\n');
        }
        qCDebug(voiceNoteLog) << "Document converted to block, text length:" << block->blockText.length();
    }
}

/**
 * @brief Utils::blockToDocument
 * @param block 绑定的数据
 * @param doc 编辑器文档
 */
void Utils::blockToDocument(const VNoteBlock *block, QTextDocument *doc)
{
    qCDebug(voiceNoteLog) << "Converting block to document";
    
    if (block && doc) {
        doc->setPlainText(block->blockText);
        qCDebug(voiceNoteLog) << "Block converted to document, text length:" << block->blockText.length();
    }
}

/**
 * @brief Utils::setDefaultColor
 * @param srcDoc 编辑器文档
 * @param color 字体颜色
 */
void Utils::setDefaultColor(QTextDocument *srcDoc, const QColor &color)
{
    qCDebug(voiceNoteLog) << "Setting default color:" << color.name();
    
    QTextCursor cursor(srcDoc);
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    QTextCharFormat newFormat = cursor.charFormat();
    newFormat.setForeground(color);
    cursor.mergeCharFormat(newFormat);
}

/**
 * @brief Utils::setTitleBarTabFocus
 * @param event
 */
void Utils::setTitleBarTabFocus(QKeyEvent *event)
{
    // qInfo() << "Setting title bar tab focus";
    // VNoteApplication *app = dynamic_cast<VNoteApplication *>(qGuiApp);
    // if (app) {
    //     VNoteMainWindow *wnd = app->mainWindow();
    //     if (wnd) {
    //         wnd->setTitleBarTabFocus(event);
    //     }
    // }
}

/**
 * @brief Utils::pictureToBase64
 * 图片转base64
 * @param imgPath 图片路径
 * @param base64 base64编码
 * @return  转换是否成功 true:成功， false:失败
 */
bool Utils::pictureToBase64(QString imgPath, QString &base64)
{
    qCDebug(voiceNoteLog) << "Converting picture to base64:" << imgPath;
    
    //加载图片，目前的参考格式为（bmp, jpg, png, jpeg, gif, ico）
    QImage img(imgPath, "bmp, jpg, png, jpeg, gif, ico");
    //非本地文件返回空字符串
    if (img.isNull()) {
        qCWarning(voiceNoteLog) << "Failed to load image:" << imgPath;
        return false;
    }

    QByteArray ba;
    QBuffer buf(&ba);
    QFileInfo fileInfo(imgPath);

    //设置图片最大宽度
    int width = qMin(img.width(), 712);
    int height = img.height();
    qCDebug(voiceNoteLog) << "Image dimensions:" << width << "x" << height;

    //等比例缩放图片
    img.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation).save(&buf, qPrintable(fileInfo.suffix()));
    //图片转base64
    base64 = QString("data:image/%1;base64," + ba.toBase64()).arg(fileInfo.suffix());
    qCDebug(voiceNoteLog) << "Image converted to base64, size:" << base64.length();
    return true;
}

bool Utils::isLoongsonPlatform()
{
    qCDebug(voiceNoteLog) << "Checking if platform is Loongson";
    
    static QString cpuModeName = "";
    if (cpuModeName.isEmpty()) {
        QProcess process;
        //获取CPU型号
        process.start("cat /proc/cpuinfo");
        if (process.waitForFinished()) {
            QString result = process.readAllStandardOutput();
            if (result.contains("Loongson")) {
                cpuModeName = "Loongson";
                qCDebug(voiceNoteLog) << "Platform is Loongson";
                return true;
            }
            cpuModeName = "other";
            qCDebug(voiceNoteLog) << "Platform is not Loongson";
        }
    }
    return cpuModeName.contains("Loongson");
}

/**
 * @brief filteredFileName
 * 过滤不符合文件名规范的文件名内容，若过滤后结果为空则返回defaultName
 * @param fileName 待过滤的文件名
 * @return 过滤后的文件名
 */
QString Utils::filteredFileName(QString fileName, const QString &defaultName)
{
    qCDebug(voiceNoteLog) << "Filtering file name:" << fileName << "default:" << defaultName;
    //使用正则表达式除去不符合规范的字符并清理文件名两端的空白字符，中间的空白字符不做处理
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    QString name = fileName.replace(QRegularExpression("[\"\'\\[\\]\\\\/:|<>+=;,?*]"), "").trimmed();
#else
    QString name = fileName.replace(QRegExp("[\"\'\\[\\]\\\\/:|<>+=;,?*]"), "").trimmed();
#endif
    if (name.isEmpty()) {
        return defaultName;
    }
    return name;
}

bool Utils::isWayland()
{
    // qInfo() << "Checking if platform is Wayland";
    return false;
    // return qApp->platformName() == "dwayland" || qApp->property("_d_isDwayland").toBool();
}

QString Utils::createRichText(const QString &title, const QString &key)
{
    if (key.isEmpty()) {
        return title.toHtmlEscaped();
    }

    const DPalette palette = DGuiApplicationHelper::instance()->applicationPalette();
    const QString highlightColor = palette.color(DPalette::Active, DPalette::Highlight).name();
    const QString highlightOpen = QStringLiteral("<span style=\"color: %1;\">").arg(highlightColor);

    QString html;
    int pos = 0;
    int match = title.indexOf(key, pos, Qt::CaseInsensitive);
    while (match >= 0) {
        html += title.mid(pos, match - pos).toHtmlEscaped();
        html += highlightOpen
            + title.mid(match, key.size()).toHtmlEscaped()
            + QStringLiteral("</span>");
        pos = match + key.size();
        match = title.indexOf(key, pos, Qt::CaseInsensitive);
    }
    html += title.mid(pos).toHtmlEscaped();
    return html;
}

QString Utils::stripHtmlTags(const QString &htmlText)
{
    qCDebug(voiceNoteLog) << "Stripping HTML tags from text:" << htmlText;
    QTextDocument doc;
    doc.setHtml(htmlText);
    QString plainText = doc.toPlainText();
    qCDebug(voiceNoteLog) << "Converted to plain text:" << plainText;
    return plainText;
}

bool Utils::inLinglongEnv()
{
    // qInfo() << "Checking if in Linglong environment";
    return !qgetenv("LINGLONG_APPID").isEmpty();
}

bool Utils::checkOsBuildValid(const QString& str)
{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    static QRegularExpression regex("^[1-9A-Z][1-9A-Z][0-9A-Z][1-9A-Z][1-9A-Z](\\.\\w+)*$");
    return regex.match(str).hasMatch();
#else
    static QRegExp regex("^[1-9A-Z][1-9A-Z][0-9A-Z][1-9A-Z][1-9A-Z](\\.\\w+)*$");
    return regex.exactMatch(str);
#endif
}

DSysInfo::UosEdition Utils::uosEditionType()
{
    static DSysInfo::UosEdition cachedEdition = DSysInfo::UosEdition::UosEditionUnknown;
    static bool hasQueried = false;
    if (hasQueried)
        return cachedEdition;

    QDBusInterface interface("org.deepin.dde.SystemInfo1",
                            "/org/deepin/dde/SystemInfo1",
                            "org.deepin.dde.SystemInfo1",
                            QDBusConnection::sessionBus());

    if(!interface.isValid()) {
        hasQueried = true;
        return cachedEdition;
    }

    QVariant osBuild = interface.property("OsBuild");
    if(!osBuild.isValid()) {
        hasQueried = true;
        return cachedEdition;
    }

    const QString buildType = osBuild.toString();
    if(buildType.isEmpty()) {
        hasQueried = true;
        return cachedEdition;
    }

    const QStringList parts = buildType.split(".");
    if(parts.size() != 3) {
        hasQueried = true;
        return cachedEdition;
    }

    cachedEdition = parseOsBuildType(parts[0]);
    hasQueried = true;
    return cachedEdition;
}

DSysInfo::UosEdition Utils::parseOsBuildType(const QString & osBuild)
{
    if (!checkOsBuildValid(osBuild)) {
        return DSysInfo::UosEdition::UosEditionUnknown;
    }

    int uosType = osBuild[1].digitValue();
    int uosEditType = osBuild[3].digitValue();

    if(uosType == 1) {
        switch (uosEditType) {
        case 1:
            return DSysInfo::UosEdition::UosProfessional;
        case 2:
        case 7:
            return DSysInfo::UosEdition::UosHome;
        case 3:
            return DSysInfo::UosEdition::UosCommunity;
        case 4:
        case 9:
            return DSysInfo::UosEdition::UosMilitary;
        case 5:
            return DSysInfo::UosEdition::UosDeviceEdition;
        case 6:
            return DSysInfo::UosEdition::UosEducation;
        default:
            break;
        }
    } else if(uosType == 2) {
        switch (uosEditType) {
        case 1:
            return DSysInfo::UosEdition::UosEnterprise;
        case 2:
            return DSysInfo::UosEdition::UosEnterpriseC;
        case 3:
            return DSysInfo::UosEdition::UosEuler;
        case 4:
        case 9:
            return DSysInfo::UosEdition::UosMilitaryS;
        case 5:
            return DSysInfo::UosEdition::UosDeviceEdition;
        default:
            break;
        }
    } else if (uosType == 3) {
        return DSysInfo::UosEdition::UosEnterprise;
    }

    return DSysInfo::UosEdition::UosEditionUnknown;
}

bool Utils::isCommunityEdition()
{
    return uosEditionType() == DSysInfo::UosEdition::UosCommunity;
}

QString Utils::makeVoiceRelative(const QString &voicePath)
{
    const QString appData = QDir::toNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    if (appData.isEmpty())
        return voicePath;
    const QString v = QDir::toNativeSeparators(voicePath);
    const QString voiceRoot = QDir::toNativeSeparators(QDir::cleanPath(QDir(appData).filePath(QStringLiteral("voicenote"))));
    const QString voiceRootWithSep = voiceRoot.endsWith(QDir::separator()) ? voiceRoot : voiceRoot + QDir::separator();
    if (v.startsWith(voiceRootWithSep))
        return QStringLiteral("voicenote/") + v.mid(voiceRootWithSep.size());
    return voicePath;
}
