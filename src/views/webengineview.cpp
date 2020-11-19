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
#include "webengineview.h"
#include "common/vnoteitem.h"
#include "common/vnoteforlder.h"
#include "db/vnoteitemoper.h"
#include "common/vnotedatamanager.h"
#include "common/actionmanager.h"
#include "common/metadataparser.h"
#include "common/jscontent.h"
#include <QDebug>
WebEngineView::WebEngineView(QWidget *parent) :
    QWebEngineView(parent)
{
     init();
}

void WebEngineView::init()
{
    m_jsContent = JsContent::instance();
    m_channel = new QWebChannel(this);
    m_channel->registerObject("webobj", m_jsContent);
    page()->setWebChannel(m_channel);
    load(QUrl("qrc:/web/index.html"));
    setContextMenuPolicy(Qt::NoContextMenu);
}

void WebEngineView::initData(VNoteItem *data, QString reg, bool fouse)
{
    if(data == nullptr){
        this->setVisible(false);
        return;
    }
    JsContent::instance()->setNoteItem(data);
    m_noteData = data;
    this->setVisible(true);
    qDebug() << data->metaDataRef().toString();
    emit m_jsContent->initData(data->metaDataRef().toString());
}

void WebEngineView::insertVoiceItem(const QString &voicePath, qint64 voiceSize)
{
    VNoteItemOper noteOps(m_noteData);
    VNoteBlock *data = m_noteData->newBlock(VNoteBlock::Voice);
    data->ptrVoice->voiceSize = voiceSize;
    data->ptrVoice->voicePath = voicePath;
    data->ptrVoice->createTime = QDateTime::currentDateTime();
    data->ptrVoice->voiceTitle = noteOps.getDefaultVoiceName();

    if(m_noteTmp == nullptr){
        m_noteTmp = new VNoteItem;
    }
    m_noteData->addBlock(data);
    m_noteTmp->datas.datas.push_back(data);

    data = m_noteData->newBlock(VNoteBlock::Text);
    m_noteData->addBlock(data);
    m_noteTmp->datas.datas.push_back(data);
    if (!noteOps.updateNote()) {
        qInfo() << "Save note error";
    }

    MetaDataParser parse;
    QVariant value;
    parse.makeMetaData(m_noteTmp, value);
    m_noteTmp->datas.datas.clear();

    qInfo() << value.toString();
    emit m_jsContent->insertVoiceItem(value.toString());
}
