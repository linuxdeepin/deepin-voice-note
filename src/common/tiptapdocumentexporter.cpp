// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "tiptapdocumentexporter.h"

#include "utils.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <QUrl>

namespace {

constexpr int kTiptapSchemaVersion = 1;

const QSet<QString> kRuntimeVoiceAttrs = {
    QStringLiteral("playing"),
    QStringLiteral("paused"),
    QStringLiteral("progress"),
    QStringLiteral("duration"),
    QStringLiteral("translating"),
    QStringLiteral("unplayable"),
    QStringLiteral("seeking"),
    QStringLiteral("current"),
};

QString htmlDocumentHead()
{
    return QStringLiteral(
        "<!DOCTYPE html>"
        "<html lang=\"zh-CN\">"
        "<head>"
        "<meta charset=\"UTF-8\">"
        "<meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
        "<title>Document</title>"
        "<style>"
        "body{font-family:'Helvetica','Noto Sans CJK SC',sans-serif;font-size:14px;"
        "line-height:1.72;max-width:780px;margin:0 auto;padding:0;word-wrap:break-word;}"
        "p{margin:0 0 8px;}h1,h2,h3,h4,h5,h6{margin:16px 0 8px;}"
        "blockquote{margin:8px 0;padding-left:12px;border-left:3px solid #d0d0d0;color:#5f6368;}"
        "ul,ol{padding-left:24px;}li>p{margin:0;}"
        "ul[data-type='taskList']{list-style:none;padding-left:0;}"
        "li[data-type='taskItem']{list-style:none;}"
        "img{max-width:100%;height:auto;}"
        ".voiceBox{background:rgba(0,0,0,.05);border-radius:12px;margin:10px 0;overflow:hidden;}"
        ".voicePlayback{min-height:42px;display:flex;align-items:center;justify-content:space-between;padding:0 12px;}"
        ".voiceTitle{font-size:14px;color:#001a2e;}"
        ".voiceTime{font-size:12px;color:rgba(65,77,104,1);margin-left:12px;}"
        ".translate{border-top:1px solid rgba(0,0,0,.06);padding:8px 20px 12px;}"
        ".translateHeader{font-size:12px;color:rgba(65,77,104,1);margin-bottom:6px;}"
        ".translateText{white-space:pre-wrap;word-break:break-word;}"
        "</style>"
        "</head><body><div class=\"note-editable\" contenteditable=\"false\">");
}

QString htmlDocumentTail()
{
    return QStringLiteral("</div></body></html>");
}

bool parseTiptapPayload(const QString &payload, QJsonObject *document)
{
    QJsonParseError parseError;
    const QJsonDocument json = QJsonDocument::fromJson(payload.trimmed().toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !json.isObject()) {
        return false;
    }

    const QJsonObject root = json.object();
    QJsonObject content;
    if (root.value(QStringLiteral("format")).toString() == QStringLiteral("tiptap")) {
        const QJsonValue versionValue = root.value(QStringLiteral("schemaVersion"));
        if (!versionValue.isDouble() || versionValue.toInt() != kTiptapSchemaVersion) {
            return false;
        }
        content = root.value(QStringLiteral("content")).toObject();
    } else if (root.value(QStringLiteral("type")).toString() == QStringLiteral("doc")) {
        content = root;
    } else {
        return false;
    }

    if (content.value(QStringLiteral("type")).toString() != QStringLiteral("doc")) {
        return false;
    }

    if (document) {
        *document = content;
    }
    return true;
}

QString textValue(const QJsonObject &object, const QString &key)
{
    const QJsonValue value = object.value(key);
    return value.isString() ? value.toString() : QString();
}

QString htmlAttr(QString value)
{
    return value.toHtmlEscaped().replace(QLatin1Char('\n'), QLatin1Char(' '));
}

QString safeCssValue(QString value)
{
    value = value.trimmed();
    value.remove(QRegularExpression(QStringLiteral("[;{}<>\\\"]")));
    return value;
}

QString sanitizedImageRelPath(const QString &relPath)
{
    const QString normalized = QDir::cleanPath(QDir::fromNativeSeparators(relPath.trimmed()));
    if (!normalized.startsWith(QStringLiteral("images/"))
        || normalized.split(QLatin1Char('/')).contains(QStringLiteral(".."))) {
        return QString();
    }
    return normalized;
}

QString localImagePath(const QJsonObject &attrs)
{
    QString relPath = sanitizedImageRelPath(textValue(attrs, QStringLiteral("relPath")));
    if (relPath.isEmpty()) {
        const QUrl url(textValue(attrs, QStringLiteral("src")));
        if (url.isLocalFile()) {
            const QString filePath = QDir::cleanPath(QDir::fromNativeSeparators(url.toLocalFile()));
            const QString appData = QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
            const QString imageRoot = QDir(appData).filePath(QStringLiteral("images"));
            const QString imageRootWithSep = QDir::cleanPath(imageRoot) + QLatin1Char('/');
            if (filePath.startsWith(imageRootWithSep)) {
                relPath = QStringLiteral("images/") + filePath.mid(imageRootWithSep.size());
            }
        }
    }
    if (relPath.isEmpty()) {
        return QString();
    }

    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appData.isEmpty()) {
        return QString();
    }
    return QDir::cleanPath(QDir(appData).filePath(relPath));
}

bool isSafeExportImageSrc(const QString &src)
{
    const QString trimmed = src.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }

    const QString normalized = QString(trimmed).remove(QRegularExpression(QStringLiteral("[\\x00-\\x20\\x7f\\s]")));
    if (normalized.isEmpty()
        || normalized.startsWith(QStringLiteral("//"))
        || normalized.startsWith(QStringLiteral("\\\\"))) {
        return false;
    }

    const QRegularExpression schemeRe(QStringLiteral("^([a-z][a-z0-9+.-]*):"), QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch schemeMatch = schemeRe.match(normalized);
    if (!schemeMatch.hasMatch()) {
        return true;
    }

    const QString scheme = schemeMatch.captured(1).toLower();
    if (scheme == QStringLiteral("http") || scheme == QStringLiteral("https")) {
        return true;
    }
    return scheme == QStringLiteral("data") && normalized.startsWith(QStringLiteral("data:image/"), Qt::CaseInsensitive);
}

QString imageSrcForHtml(const QJsonObject &attrs)
{
    const QString path = localImagePath(attrs);
    if (!path.isEmpty()) {
        QString base64;
        if (Utils::pictureToBase64(path, base64)) {
            return base64;
        }
    }

    const QString src = textValue(attrs, QStringLiteral("src"));
    if (isSafeExportImageSrc(src)) {
        return src.trimmed();
    }

    return sanitizedImageRelPath(textValue(attrs, QStringLiteral("relPath")));
}

QString imageLabel(const QJsonObject &attrs)
{
    const QString title = textValue(attrs, QStringLiteral("title"));
    if (!title.trimmed().isEmpty()) {
        return title.trimmed();
    }
    const QString alt = textValue(attrs, QStringLiteral("alt"));
    if (!alt.trimmed().isEmpty()) {
        return alt.trimmed();
    }
    const QString relPath = textValue(attrs, QStringLiteral("relPath"));
    if (!relPath.trimmed().isEmpty()) {
        return QFileInfo(relPath).fileName();
    }
    return QStringLiteral("image");
}

QString voiceTitle(const QJsonObject &attrs)
{
    const QString title = textValue(attrs, QStringLiteral("title"));
    if (!title.trimmed().isEmpty()) {
        return title.trimmed();
    }
    const QString path = textValue(attrs, QStringLiteral("voicePath"));
    if (!path.trimmed().isEmpty()) {
        return QFileInfo(path).completeBaseName();
    }
    return QStringLiteral("voice");
}

QString voiceCreateTime(const QJsonObject &attrs)
{
    return textValue(attrs, QStringLiteral("createTime"));
}

QJsonObject persistedVoiceAttrs(const QJsonObject &attrs)
{
    QJsonObject persisted;
    for (auto it = attrs.constBegin(); it != attrs.constEnd(); ++it) {
        if (kRuntimeVoiceAttrs.contains(it.key())) {
            continue;
        }
        persisted.insert(it.key(), it.value());
    }
    return persisted;
}

QString plainInline(const QJsonObject &node);
QString plainBlock(const QJsonObject &node);
QString htmlInline(const QJsonObject &node);
QString htmlBlock(const QJsonObject &node);
void collectVoiceJsons(const QJsonObject &node, QStringList *voices);

QString joinPlainBlocks(const QJsonArray &children)
{
    QStringList blocks;
    for (const QJsonValue &child : children) {
        if (!child.isObject()) {
            continue;
        }
        const QString text = plainBlock(child.toObject()).trimmed();
        if (!text.isEmpty()) {
            blocks.append(text);
        }
    }
    return blocks.join(QStringLiteral("\n"));
}

QString plainChildren(const QJsonObject &node)
{
    QString text;
    const QJsonArray children = node.value(QStringLiteral("content")).toArray();
    for (const QJsonValue &child : children) {
        if (child.isObject()) {
            text += plainInline(child.toObject());
        }
    }
    return text;
}

QString plainBlockChildren(const QJsonObject &node)
{
    QStringList blocks;
    const QJsonArray children = node.value(QStringLiteral("content")).toArray();
    for (const QJsonValue &child : children) {
        if (!child.isObject()) {
            continue;
        }
        const QString text = plainBlock(child.toObject()).trimmed();
        if (!text.isEmpty()) {
            blocks.append(text);
        }
    }
    return blocks.join(QStringLiteral("\n"));
}

QString withListMarker(QString text, const QString &marker)
{
    text = text.trimmed();
    if (text.isEmpty()) {
        return QString();
    }
    return marker + text.replace(QStringLiteral("\n"), QStringLiteral("\n") + QString(marker.size(), QLatin1Char(' ')));
}

QString plainInline(const QJsonObject &node)
{
    const QString type = node.value(QStringLiteral("type")).toString();
    if (type == QStringLiteral("text")) {
        return textValue(node, QStringLiteral("text"));
    }
    if (type == QStringLiteral("hardBreak")) {
        return QStringLiteral("\n");
    }
    if (type == QStringLiteral("image")) {
        return QStringLiteral("[图片: %1]").arg(imageLabel(node.value(QStringLiteral("attrs")).toObject()));
    }
    if (type == QStringLiteral("voiceBlock")) {
        return plainBlock(node);
    }
    return plainChildren(node);
}

QString plainBlock(const QJsonObject &node)
{
    const QString type = node.value(QStringLiteral("type")).toString();
    if (type == QStringLiteral("doc")) {
        return joinPlainBlocks(node.value(QStringLiteral("content")).toArray());
    }
    if (type == QStringLiteral("paragraph") || type == QStringLiteral("heading")) {
        return plainChildren(node);
    }
    if (type == QStringLiteral("blockquote") || type == QStringLiteral("listItem")) {
        return plainBlockChildren(node);
    }
    if (type == QStringLiteral("taskItem")) {
        const QString marker = node.value(QStringLiteral("attrs")).toObject().value(QStringLiteral("checked")).toBool(false)
            ? QStringLiteral("[x] ") : QStringLiteral("[ ] ");
        return withListMarker(plainBlockChildren(node), marker);
    }
    if (type == QStringLiteral("bulletList") || type == QStringLiteral("orderedList") || type == QStringLiteral("taskList")) {
        QStringList items;
        int index = 1;
        const QJsonArray children = node.value(QStringLiteral("content")).toArray();
        for (const QJsonValue &child : children) {
            if (!child.isObject()) {
                continue;
            }
            QString text = plainBlock(child.toObject()).trimmed();
            if (text.isEmpty()) {
                continue;
            }
            if (type == QStringLiteral("bulletList")) {
                text = withListMarker(text, QStringLiteral("- "));
            } else if (type == QStringLiteral("orderedList")) {
                text = withListMarker(text, QString::number(index++) + QStringLiteral(". "));
            }
            if (!text.isEmpty()) {
                items.append(text);
            }
        }
        return items.join(QStringLiteral("\n"));
    }
    if (type == QStringLiteral("image")) {
        return QStringLiteral("[图片: %1]").arg(imageLabel(node.value(QStringLiteral("attrs")).toObject()));
    }
    if (type == QStringLiteral("voiceBlock")) {
        const QJsonObject attrs = persistedVoiceAttrs(node.value(QStringLiteral("attrs")).toObject());
        // TXT 导出面向用户可读内容：语音节点只导出转写正文。
        // 录音标题、voiceId、路径等结构元数据由 HTML/语音文件导出表达，
        // 不能混入文本结果形成“[语音: xxx]”这类伪正文。
        return textValue(attrs, QStringLiteral("text"));
    }
    return plainChildren(node);
}

QString markOpenHtml(const QJsonObject &mark)
{
    const QString type = mark.value(QStringLiteral("type")).toString();
    const QJsonObject attrs = mark.value(QStringLiteral("attrs")).toObject();
    if (type == QStringLiteral("bold")) return QStringLiteral("<strong>");
    if (type == QStringLiteral("italic")) return QStringLiteral("<em>");
    if (type == QStringLiteral("underline")) return QStringLiteral("<u>");
    if (type == QStringLiteral("strike")) return QStringLiteral("<s>");
    if (type == QStringLiteral("color")) {
        const QString color = safeCssValue(textValue(attrs, QStringLiteral("color")));
        return color.isEmpty() ? QString() : QStringLiteral("<span style=\"color:%1\">").arg(htmlAttr(color));
    }
    if (type == QStringLiteral("highlight")) {
        const QString color = safeCssValue(textValue(attrs, QStringLiteral("color")));
        return color.isEmpty() ? QStringLiteral("<mark>") : QStringLiteral("<mark style=\"background-color:%1\">").arg(htmlAttr(color));
    }
    if (type == QStringLiteral("fontFamily")) {
        const QString family = safeCssValue(textValue(attrs, QStringLiteral("fontFamily")));
        return family.isEmpty() ? QString() : QStringLiteral("<span style=\"font-family:%1\">").arg(htmlAttr(family));
    }
    if (type == QStringLiteral("fontSize")) {
        const QString size = safeCssValue(textValue(attrs, QStringLiteral("fontSize")));
        return size.isEmpty() ? QString() : QStringLiteral("<span style=\"font-size:%1\">").arg(htmlAttr(size));
    }
    return QString();
}

QString markCloseHtml(const QJsonObject &mark)
{
    const QString type = mark.value(QStringLiteral("type")).toString();
    if (type == QStringLiteral("bold")) return QStringLiteral("</strong>");
    if (type == QStringLiteral("italic")) return QStringLiteral("</em>");
    if (type == QStringLiteral("underline")) return QStringLiteral("</u>");
    if (type == QStringLiteral("strike")) return QStringLiteral("</s>");
    if (type == QStringLiteral("color") || type == QStringLiteral("fontFamily") || type == QStringLiteral("fontSize")) return QStringLiteral("</span>");
    if (type == QStringLiteral("highlight")) return QStringLiteral("</mark>");
    return QString();
}

QString applyMarks(QString html, const QJsonArray &marks)
{
    QString prefix;
    QString suffix;
    for (const QJsonValue &value : marks) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject mark = value.toObject();
        const QString open = markOpenHtml(mark);
        const QString close = markCloseHtml(mark);
        if (open.isEmpty() || close.isEmpty()) {
            continue;
        }
        prefix += open;
        suffix.prepend(close);
    }
    return prefix + html + suffix;
}

QString htmlChildren(const QJsonObject &node)
{
    QString html;
    const QJsonArray children = node.value(QStringLiteral("content")).toArray();
    for (const QJsonValue &child : children) {
        if (child.isObject()) {
            html += htmlInline(child.toObject());
        }
    }
    return html;
}

QString htmlInline(const QJsonObject &node)
{
    const QString type = node.value(QStringLiteral("type")).toString();
    if (type == QStringLiteral("text")) {
        QString html = textValue(node, QStringLiteral("text")).toHtmlEscaped();
        html.replace(QLatin1Char('\n'), QStringLiteral("<br>"));
        return applyMarks(html, node.value(QStringLiteral("marks")).toArray());
    }
    if (type == QStringLiteral("hardBreak")) {
        return QStringLiteral("<br>");
    }
    if (type == QStringLiteral("image")) {
        const QJsonObject attrs = node.value(QStringLiteral("attrs")).toObject();
        const QString src = imageSrcForHtml(attrs);
        if (src.trimmed().isEmpty()) {
            return QString();
        }
        QString html = QStringLiteral("<img src=\"%1\"").arg(htmlAttr(src));
        const QString alt = textValue(attrs, QStringLiteral("alt"));
        const QString title = textValue(attrs, QStringLiteral("title"));
        if (!alt.isEmpty()) html += QStringLiteral(" alt=\"%1\"").arg(htmlAttr(alt));
        if (!title.isEmpty()) html += QStringLiteral(" title=\"%1\"").arg(htmlAttr(title));
        html += QStringLiteral(">");
        return html;
    }
    if (type == QStringLiteral("voiceBlock")) {
        return htmlBlock(node);
    }
    return htmlChildren(node);
}

QString blockContentOrBreak(const QJsonObject &node)
{
    const QString html = htmlChildren(node);
    return html.isEmpty() ? QStringLiteral("<br>") : html;
}

QString htmlBlockChildren(const QJsonObject &node)
{
    QString html;
    const QJsonArray children = node.value(QStringLiteral("content")).toArray();
    for (const QJsonValue &child : children) {
        if (child.isObject()) {
            html += htmlBlock(child.toObject());
        }
    }
    return html;
}

QString htmlBlock(const QJsonObject &node)
{
    const QString type = node.value(QStringLiteral("type")).toString();
    if (type == QStringLiteral("doc")) {
        QString html;
        const QJsonArray children = node.value(QStringLiteral("content")).toArray();
        for (const QJsonValue &child : children) {
            if (child.isObject()) {
                html += htmlBlock(child.toObject());
            }
        }
        return html;
    }
    if (type == QStringLiteral("paragraph")) {
        return QStringLiteral("<p>%1</p>").arg(blockContentOrBreak(node));
    }
    if (type == QStringLiteral("heading")) {
        const int level = qBound(1, node.value(QStringLiteral("attrs")).toObject().value(QStringLiteral("level")).toInt(1), 6);
        return QStringLiteral("<h%1>%2</h%1>").arg(level).arg(blockContentOrBreak(node));
    }
    if (type == QStringLiteral("blockquote")) {
        return QStringLiteral("<blockquote>%1</blockquote>").arg(blockContentOrBreak(node));
    }
    if (type == QStringLiteral("bulletList")) {
        return QStringLiteral("<ul>%1</ul>").arg(htmlBlockChildren(node));
    }
    if (type == QStringLiteral("orderedList")) {
        return QStringLiteral("<ol>%1</ol>").arg(htmlBlockChildren(node));
    }
    if (type == QStringLiteral("taskList")) {
        return QStringLiteral("<ul data-type=\"taskList\">%1</ul>").arg(htmlBlockChildren(node));
    }
    if (type == QStringLiteral("listItem")) {
        return QStringLiteral("<li>%1</li>").arg(htmlBlockChildren(node));
    }
    if (type == QStringLiteral("taskItem")) {
        const bool checked = node.value(QStringLiteral("attrs")).toObject().value(QStringLiteral("checked")).toBool(false);
        return QStringLiteral("<li data-type=\"taskItem\" data-checked=\"%1\"><input type=\"checkbox\" disabled%2> %3</li>")
            .arg(checked ? QStringLiteral("true") : QStringLiteral("false"),
                 checked ? QStringLiteral(" checked") : QString(),
                 htmlBlockChildren(node));
    }
    if (type == QStringLiteral("image")) {
        return QStringLiteral("<p>%1</p>").arg(htmlInline(node));
    }
    if (type == QStringLiteral("voiceBlock")) {
        const QJsonObject attrs = persistedVoiceAttrs(node.value(QStringLiteral("attrs")).toObject());
        const QString title = voiceTitle(attrs).toHtmlEscaped();
        const QString createTime = voiceCreateTime(attrs).toHtmlEscaped();
        const QString transcript = textValue(attrs, QStringLiteral("text"));
        QString html = QStringLiteral("<div class=\"voiceBox\" data-type=\"voice-block\">");
        html += QStringLiteral("<div class=\"voicePlayback\"><div class=\"voiceTitle\">%1</div>").arg(title);
        if (!createTime.isEmpty()) {
            html += QStringLiteral("<div class=\"voiceTime\">%1</div>").arg(createTime);
        }
        html += QStringLiteral("</div>");
        if (!transcript.trimmed().isEmpty()) {
            html += QStringLiteral("<div class=\"translate\"><div class=\"translateHeader\">语音转文字</div><div class=\"translateText\">%1</div></div>")
                        .arg(transcript.toHtmlEscaped().replace(QLatin1Char('\n'), QStringLiteral("<br>")));
        }
        html += QStringLiteral("</div>");
        return html;
    }
    return htmlChildren(node);
}

void collectVoiceJsons(const QJsonObject &node, QStringList *voices)
{
    if (!voices) {
        return;
    }
    const QString type = node.value(QStringLiteral("type")).toString();
    if (type == QStringLiteral("voiceBlock")) {
        const QJsonObject attrs = persistedVoiceAttrs(node.value(QStringLiteral("attrs")).toObject());
        const QString voicePath = textValue(attrs, QStringLiteral("voicePath"));
        if (!voicePath.trimmed().isEmpty()) {
            QJsonObject legacy;
            legacy.insert(QStringLiteral("type"), 2);
            legacy.insert(QStringLiteral("voiceId"), textValue(attrs, QStringLiteral("voiceId")));
            legacy.insert(QStringLiteral("voicePath"), voicePath);
            legacy.insert(QStringLiteral("voiceSize"), attrs.value(QStringLiteral("voiceSize")).toInt(0));
            legacy.insert(QStringLiteral("createTime"), textValue(attrs, QStringLiteral("createTime")));
            legacy.insert(QStringLiteral("title"), voiceTitle(attrs));
            const QString transcript = textValue(attrs, QStringLiteral("text"));
            legacy.insert(QStringLiteral("text"), transcript);
            legacy.insert(QStringLiteral("state"), !transcript.trimmed().isEmpty());
            voices->append(QString::fromUtf8(QJsonDocument(legacy).toJson(QJsonDocument::Compact)));
        }
        return;
    }

    const QJsonArray children = node.value(QStringLiteral("content")).toArray();
    for (const QJsonValue &child : children) {
        if (child.isObject()) {
            collectVoiceJsons(child.toObject(), voices);
        }
    }
}

} // namespace

bool TiptapDocumentExporter::isTiptapEnvelope(const QString &payload)
{
    return parseTiptapPayload(payload, nullptr);
}

QString TiptapDocumentExporter::toPlainText(const QString &payload)
{
    QJsonObject document;
    if (!parseTiptapPayload(payload, &document)) {
        return QString();
    }
    return plainBlock(document);
}

QString TiptapDocumentExporter::toHtml(const QString &payload)
{
    QJsonObject document;
    if (!parseTiptapPayload(payload, &document)) {
        return QString();
    }
    return htmlDocumentHead() + htmlBlock(document) + htmlDocumentTail();
}

QStringList TiptapDocumentExporter::voiceJsons(const QString &payload)
{
    QJsonObject document;
    if (!parseTiptapPayload(payload, &document)) {
        return {};
    }
    QStringList voices;
    collectVoiceJsons(document, &voices);
    return voices;
}

bool TiptapDocumentExporter::hasText(const QString &payload)
{
    return !toPlainText(payload).trimmed().isEmpty();
}

bool TiptapDocumentExporter::hasVoice(const QString &payload)
{
    return !voiceJsons(payload).isEmpty();
}
