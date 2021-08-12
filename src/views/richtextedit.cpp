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

static const char webPage[] = WEB_PATH "/index.html";

RichTextEdit::RichTextEdit(QWidget *parent)
    : QWebEngineView(parent)
{
    m_channel = new QWebChannel(this);
    m_jsContent = JsContent::instance();
    m_channel->registerObject("webobj", m_jsContent);
    page()->setWebChannel(m_channel);
    QFileInfo info(webPage);
    load(QUrl::fromLocalFile(info.absoluteFilePath()));
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(1000);
    connect(m_updateTimer, &QTimer::timeout, this, &RichTextEdit::updateNote);
    connect(m_jsContent, &JsContent::textChange, this, &RichTextEdit::onTextChange);
}

void RichTextEdit::initData(VNoteItem *data, const QString &reg, bool fouse)
{
    Q_UNUSED(reg)
    m_updateTimer->stop();
    if (fouse) {
        setFocus();
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
    if (m_noteData) {
        if (m_textChange || m_noteData->htmlCode.isEmpty()) {
            QVariant result = m_jsContent->callJsSynchronous(page(), QString("getHtml()"));
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

void RichTextEdit::onTextChange()
{
    m_textChange = true;
}
