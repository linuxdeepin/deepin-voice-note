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
#include "common/setting.h"
#include "common/actionmanager.h"
#include "common/vtextspeechandtrmanager.h"
#include "common/jscontent.h"

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
    connect(m_updateTimer, &QTimer::timeout, this, [ = ] {
        updateNote();
    });
    m_updateTimer->setInterval(1000);
    m_jsContent = JsContent::instance();
    m_jsContent->setWebPage(page());
    connect(m_jsContent, &JsContent::textChange, this, [=]{
        if(m_textChange == false && m_noteData){
            m_textChange = true;
            m_noteData->modifyTime = QDateTime::currentDateTime();
            emit contentChanged();
        }
    });
    m_channel = new QWebChannel(this);
    m_channel->registerObject("webobj", m_jsContent);
    page()->setWebChannel(m_channel);
    const QFileInfo info(webPage);
    load(QUrl::fromLocalFile(info.absoluteFilePath()));
    m_noteDetailContextMenu = ActionManager::Instance()->detialContextMenu();
}

void WebEngineView::updateNote()
{
    if (m_noteData) {
        if(m_textChange || m_noteData->htmlCode.isEmpty()){
            QVariant result = m_jsContent->callJsSynchronous("getHtml()");
            if(result.isValid()){
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

void WebEngineView::initData(VNoteItem *data, QString reg, bool fouse)
{
    m_updateTimer->stop();
    updateNote();
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
    findText(reg);
}

void WebEngineView::insertVoiceItem(const QString &voicePath, qint64 voiceSize)
{
    VNoteBlock *data = new VNVoiceBlock();
    data->ptrVoice->voiceSize = voiceSize;
    data->ptrVoice->voicePath = voicePath;
    data->ptrVoice->createTime = QDateTime::currentDateTime();

    int index = voicePath.lastIndexOf('/');
    data->ptrVoice->voiceTitle = data->ptrVoice->voicePath.right(voicePath.length() - index - 1);

    MetaDataParser parse;
    QVariant value;
    parse.jsonMakeMetadata(data, value);
    delete  data;
    data = nullptr;

    qInfo() << value.toString();
    this->setFocus();
    emit m_jsContent->callJsInsertVoice(value.toString());
}

void WebEngineView::contextMenuEvent(QContextMenuEvent *e)
{
    Q_UNUSED(e)
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
    if (canSelectAll) {
        ActionManager::Instance()->enableAction(ActionManager::DetailSelectAll, canSelectAll);
    }
    if (canCopy) {
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
    if (canCut) {
        ActionManager::Instance()->enableAction(ActionManager::DetailCut, true);
    }
    if (canDelete) {
        ActionManager::Instance()->enableAction(ActionManager::DetailDelete, true);
    }
    if (canPaste) {
        ActionManager::Instance()->enableAction(ActionManager::DetailPaste, true);
        if (isAISrvAvailable && VTextSpeechAndTrManager::getSpeechToTextEnable()) {
            ActionManager::Instance()->enableAction(ActionManager::DetailSpeech2Text, true);
        }
    }
    m_noteDetailContextMenu->popup(QCursor::pos());

}

void WebEngineView::saveMp3()
{
    VNVoiceBlock *block = m_jsContent->getCurrentVoice();
    if (block) {
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
                exportDir, ExportNoteWorker::ExportOneVoice, QVariant(), m_noteData, block);
            exportWorker->setAutoDelete(true);

            QThreadPool::globalInstance()->start(exportWorker);
        }
    }
}
