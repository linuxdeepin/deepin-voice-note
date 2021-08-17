/*
* Copyright (C) 2019 ~ 2019 UnionTech Software Technology Co.,Ltd.
*
* Author:     liuyanga <liuyanga@uniontech.com>
*
* Maintainer: liuyanga <liuyanga@uniontech.com>
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
#include "richtextedit.h"
#include "common/jscontent.h"
#include "common/vnoteitem.h"
#include "common/metadataparser.h"
#include "db/vnoteitemoper.h"

#include <DFileDialog>
#include <DStandardPaths>
#include <DApplication>

#include <QClipboard>
#include <QVBoxLayout>
#include <QMimeData>

static const char webPage[] = WEB_PATH "/index.html";

RichTextEdit::RichTextEdit(QWidget *parent)
    : QWidget(parent)
{
    m_jsContent = JsContent::instance();
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(1000);
    connect(m_updateTimer, &QTimer::timeout, this, &RichTextEdit::updateNote);
    connect(m_jsContent, &JsContent::textChange, this, &RichTextEdit::onTextChange);
    //初始化编辑区，ut测试中此函数打桩解决无法新建QWebEngineView问题
    initWebView();
}

void RichTextEdit::initData(VNoteItem *data, const QString &reg, bool fouse)
{
    Q_UNUSED(reg)
    m_updateTimer->stop();
    if (fouse && m_webView) {
        m_webView->setFocus();
    }
    if (data == nullptr) {
        this->setVisible(false);
        return;
    }
    m_noteData = data;
    this->setVisible(true);
    if (data->htmlCode.isEmpty()) {
        emit m_jsContent->callJsInitData(data->metaDataRef().toString());
    } else {
        emit m_jsContent->callJsSetHtml(data->htmlCode);
    }
    m_updateTimer->start();
}

void RichTextEdit::insertVoiceItem(const QString &voicePath, qint64 voiceSize)
{
    VNVoiceBlock data;
    data.ptrVoice->voiceSize = voiceSize;
    data.ptrVoice->voicePath = voicePath;
    data.ptrVoice->createTime = QDateTime::currentDateTime();
    data.ptrVoice->voiceTitle = data.ptrVoice->createTime.toString("yyyyMMdd hh.mm.ss");

    MetaDataParser parse;
    QVariant value;
    parse.makeMetaData(&data, value);
    this->setFocus();
    emit m_jsContent->callJsInsertVoice(value.toString());
}

void RichTextEdit::updateNote()
{
    if (m_noteData && m_webView) {
        if (m_textChange || m_noteData->htmlCode.isEmpty()) {
            QVariant result = m_jsContent->callJsSynchronous(m_webView->page(), QString("getHtml()"));
            if (result.isValid()) {
                m_noteData->htmlCode = result.toString();
                VNoteItemOper noteOps(m_noteData);
                if (!noteOps.updateNote()) {
                    qInfo() << "Save note error";
                }
            }
            m_textChange = false;
        }
    }
}

/**
 * @brief RichTextEdit::getImagePathsByClipboard
 * 从系统剪贴板中获取图片路径
 */
void RichTextEdit::getImagePathsByClipboard()
{
    //获取剪贴板信息
    QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();
    //存在文件url
    if (mimeData->hasUrls()) {
        QStringList paths;
        for (auto url : mimeData->urls()) {
            paths.push_back(url.path());
        }
        if (m_jsContent->insertImages(paths))
            return;
    }
    //无图片文件，直接调用web端的粘贴事件
    m_webView->page()->triggerAction(QWebEnginePage::Paste);
}

void RichTextEdit::onTextChange()
{
    m_textChange = true;
}

void RichTextEdit::initWebView()
{
    m_channel = new QWebChannel(this);
    m_channel->registerObject("webobj", m_jsContent);
    m_webView = new QWebEngineView(this);
    m_webView->page()->setWebChannel(m_channel);
    m_webView->setContextMenuPolicy(Qt::NoContextMenu);
    QFileInfo info(webPage);
    m_webView->load(QUrl::fromLocalFile(info.absoluteFilePath()));
    m_webView->setBackgroundRole(QPalette::Highlight);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_webView);
}

/**
 * @brief VNoteMainWindow::onImgInsertClicked
 * 图片插入点击事件响应
 */
void RichTextEdit::onImgInsertClicked()
{
    QStringList filePaths = DFileDialog::getOpenFileNames(
        this,
        "Please choose an image file",
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
        "Image file(*.jpg *.png *.bmp)");

    m_jsContent->insertImages(filePaths);
}

/**
 * @brief VNoteMainWindow::onPaste
 * 粘贴事件
 */
void RichTextEdit::onPaste()
{
    getImagePathsByClipboard();
}
