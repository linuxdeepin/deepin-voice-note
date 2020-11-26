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
#include "common/setting.h"
#include "common/actionmanager.h"
#include "common/vtextspeechandtrmanager.h"

#include "task/exportnoteworker.h"
#include <QTimer>
#include <QDebug>
#include <QStandardPaths>
#include <QThreadPool>
#include <QContextMenuEvent>
#include <QWebEngineContextMenuData>

#include <DFileDialog>
#include <DApplication>

//const char webPage[] = WEB_PATH "/temp/index.html";
const char webPage[] = WEB_PATH "/index.html";

WebEngineView::WebEngineView(QWidget *parent) :
    QWebEngineView(parent)
{
    init();
}

void WebEngineView::init()
{
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, [=]{
        JsContent::instance()->updateNote();
    });
    m_updateTimer->setInterval(1000);
    m_jsContent = JsContent::instance();
    m_jsContent->setPage(page());
    m_channel = new QWebChannel(this);
    m_channel->registerObject("webobj", m_jsContent);
    page()->setWebChannel(m_channel);
    const QFileInfo info(webPage);
    load(QUrl::fromLocalFile(info.absoluteFilePath()));
    //setContextMenuPolicy(Qt::NoContextMenu);
    m_noteDetailContextMenu = ActionManager::Instance()->detialContextMenu();
}

void WebEngineView::initData(VNoteItem *data, QString reg, bool fouse)
{
    if (data == nullptr) {
        this->setVisible(false);
        return;
    }
    //m_updateTimer->stop();
    m_jsContent->updateNote();
    m_jsContent->setNoteItem(data);
    m_noteData = data;
    this->setVisible(true);
    if (data->htmlCode.isEmpty()) {
        qDebug() << "initData:" << data->metaDataRef().toString();
        emit m_jsContent->initData(data->metaDataRef().toString());
    } else {
        qDebug() << "setHtml:" << data->htmlCode;
        emit m_jsContent->setHtml(data->htmlCode);
    }
    //m_updateTimer->start();
}

void WebEngineView::manualUpdate()
{
    m_updateTimer->stop();
    m_jsContent->updateNote();
    m_updateTimer->start();
}

void WebEngineView::insertVoiceItem(const QString &voicePath, qint64 voiceSize)
{
    VNoteItemOper noteOps(m_noteData);
    VNoteBlock *data = m_noteData->newBlock(VNoteBlock::Voice);
    data->ptrVoice->voiceSize = voiceSize;
    data->ptrVoice->voicePath = voicePath;
    data->ptrVoice->createTime = QDateTime::currentDateTime();
    data->ptrVoice->voiceTitle = noteOps.getDefaultVoiceName();

    if (m_noteTmp == nullptr) {
        m_noteTmp = new VNoteItem;
    }

    m_noteData->addBlock(data);
    m_noteTmp->datas.datas.push_back(data);

    MetaDataParser parse;
    QVariant value;
    parse.makeMetaData(m_noteTmp, value);
    m_noteTmp->datas.datas.clear();

    qInfo() << value.toString();
    emit m_jsContent->insertVoiceItem(value.toString());
}

void WebEngineView::deleteVoice(const QString &id)
{
    VNoteBlock *data = JsContent::instance()->getBlock(id);
    if (data) {
        emit JsContent::instance()->deleteVoice(id);
        QTimer::singleShot(0, this, [this, data]() {
            int index = m_noteData->datas.datas.indexOf(data);
            VNoteBlock *nextData = m_noteData->datas.datas[index + 1];
            data->releaseSpecificData();
            m_noteData->delBlock(data);
            if (nextData) {
                m_noteData->delBlock(nextData);
            }
        });
    }
}

void WebEngineView::contextMenuEvent(QContextMenuEvent *e)
{
    const QWebEngineContextMenuData &data = page()->contextMenuData();
    QWebEngineContextMenuData::EditFlags flags = data.editFlags();
    bool TTSisWorking = false;
    OpsStateInterface *stateInterface = OpsStateInterface::instance();
    bool isAISrvAvailable = stateInterface->isAiSrvExist();
    ActionManager::Instance()->resetCtxMenu(ActionManager::MenuType::NoteDetailCtxMenu, false);
    if (isAISrvAvailable) {
        TTSisWorking = VTextSpeechAndTrManager::isTextToSpeechInWorking();
        if (TTSisWorking) {
            ActionManager::Instance()->visibleAction(ActionManager::DetailStopreading, true);
            ActionManager::Instance()->visibleAction(ActionManager::DetailText2Speech, false);
            ActionManager::Instance()->enableAction(ActionManager::DetailStopreading, true);
        } else {
            ActionManager::Instance()->visibleAction(ActionManager::DetailStopreading, false);
            ActionManager::Instance()->visibleAction(ActionManager::DetailText2Speech, true);
        }
    }
    bool canDelete = flags.testFlag(QWebEngineContextMenuData::CanDelete);
    bool canCopy = flags.testFlag(QWebEngineContextMenuData::CanCopy);
    bool canCut = flags.testFlag(QWebEngineContextMenuData::CanCut);
    bool canPaste = flags.testFlag(QWebEngineContextMenuData::CanPaste);
    bool canSelectAll = flags.testFlag(QWebEngineContextMenuData::CanSelectAll);
    if(canSelectAll){
        ActionManager::Instance()->enableAction(ActionManager::DetailSelectAll, canSelectAll);
    }
    if(canCopy){
        ActionManager::Instance()->enableAction(ActionManager::DetailCopy, canCopy);
        if (isAISrvAvailable) {
            if (VTextSpeechAndTrManager::getTransEnable()) {
                ActionManager::Instance()->enableAction(ActionManager::DetailTranslate, true);
            }
            if (!TTSisWorking && VTextSpeechAndTrManager::getTextToSpeechEnable()) {
                ActionManager::Instance()->enableAction(ActionManager::DetailText2Speech, true);
            }
        }
    }
    if(canCut){
        ActionManager::Instance()->enableAction(ActionManager::DetailCut, true);
    }
    if(canDelete){
        ActionManager::Instance()->enableAction(ActionManager::DetailDelete, true);
    }
    if(canPaste){
       ActionManager::Instance()->enableAction(ActionManager::DetailPaste, true);
       if (isAISrvAvailable && VTextSpeechAndTrManager::getSpeechToTextEnable()) {
           ActionManager::Instance()->enableAction(ActionManager::DetailSpeech2Text, true);
       }
    }
    m_noteDetailContextMenu->exec(QCursor::pos());

}

void WebEngineView::saveMp3()
{
    VNoteBlock *block = JsContent::instance()->getCurrentBlock();
    if (block && block->blockType == VNoteBlock::Voice) {
        DFileDialog dialog;
        dialog.setFileMode(DFileDialog::DirectoryOnly);

        dialog.setLabelText(DFileDialog::Accept, DApplication::translate("RightView", "Save"));
        dialog.setNameFilter("MP3(*.mp3)");
        QString historyDir = setting::instance()->getOption(VNOTE_EXPORT_VOICE_PATH_KEY).toString();
        if (historyDir.isEmpty()) {
            historyDir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        }
        dialog.setDirectory(historyDir);
        if (QDialog::Accepted == dialog.exec()) {
            // save the directory string to config file.
            setting::instance()->setOption(VNOTE_EXPORT_VOICE_PATH_KEY, dialog.directoryUrl().toLocalFile());

            QString exportDir = dialog.directoryUrl().toLocalFile();

            ExportNoteWorker *exportWorker = new ExportNoteWorker(
                exportDir, ExportNoteWorker::ExportOneVoice, m_noteData, block);
            exportWorker->setAutoDelete(true);

            QThreadPool::globalInstance()->start(exportWorker);
        }
    }
}
