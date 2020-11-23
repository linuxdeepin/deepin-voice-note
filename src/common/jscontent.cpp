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
#include "jscontent.h"
#include "common/utils.h"
#include "common/vnoteitem.h"
#include "common/actionmanager.h"
#include "db/vnoteitemoper.h"
#include "common/opsstateinterface.h"
#include "common/vtextspeechandtrmanager.h"
#include <QDateTime>
#include <QDebug>
#include <QSemaphore>
static JsContent *jsInstance = nullptr;

JsContent::JsContent(QObject *parent) : QObject(parent)
{
    m_noteDetailContextMenu = ActionManager::Instance()->detialContextMenu();
}

QString JsContent::getVoiceSize(const QString &millisecond)
{
    return Utils::formatMillisecond(millisecond.toLong(), 0);
}

QString JsContent::getVoiceTime(const QString &time)
{
    QDateTime dataTime = QDateTime::fromString(time, "yyyy-MM-dd hh:mm:ss");;
    return  Utils::convertDateTime(dataTime);
}

int JsContent::playButtonClick(const QString &id, int status)
{
    qInfo() << "get status:" << status ;
    VNoteBlock *data = getBlock(id);
    if (data && data->getType() == VNoteBlock::Voice) {
        if (status == 0) {
            emit voicePlay(data->ptrVoice);
            return 1;
        } else {
            emit voicePause(data->ptrVoice);
            return 0;
        }
    }
    qInfo() << "can not get id:" << id;
    return -1;
}

void JsContent::setNoteItem(VNoteItem *notedata)
{
    m_notedata = notedata;
}

VNoteBlock *JsContent::getBlock(const QString &id)
{
    VNoteBlock *data = nullptr;
    if (m_notedata) {
        for (auto it : m_notedata->datas.datas) {
            if (id == it->blockid) {
                data = it;
                break;
            }
        }
    }
    return  data;
}

void JsContent::rightMenuClick(const QString &id, int select)
{
    qInfo() << "get select:" << select;
    m_currentBlock = getBlock(id);
    if (m_currentBlock) {
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
        bool canCutDel = true;
        if (m_currentBlock->getType() == VNoteBlock::Text) {
            if (select) {
                ActionManager::Instance()->enableAction(ActionManager::DetailCopy, true);
                if (isAISrvAvailable) {
                    if (VTextSpeechAndTrManager::getTransEnable()) {
                        ActionManager::Instance()->enableAction(ActionManager::DetailTranslate, true);
                    }
                    if (!TTSisWorking && VTextSpeechAndTrManager::getTextToSpeechEnable()) {
                        ActionManager::Instance()->enableAction(ActionManager::DetailText2Speech, true);
                    }
                }
                if (canCutDel) {
                    ActionManager::Instance()->enableAction(ActionManager::DetailCut, canCutDel);
                    ActionManager::Instance()->enableAction(ActionManager::DetailDelete, canCutDel);
                }
            } else {
                ActionManager::Instance()->enableAction(ActionManager::DetailPaste, true);
                if (isAISrvAvailable && VTextSpeechAndTrManager::getSpeechToTextEnable()) {
                    ActionManager::Instance()->enableAction(ActionManager::DetailSpeech2Text, true);
                }
                canCutDel = false;
            }

        } else {
            if (select) {
                ActionManager::Instance()->enableAction(ActionManager::DetailCopy, true);
                if (isAISrvAvailable) {
                    if (VTextSpeechAndTrManager::getTransEnable()) {
                        ActionManager::Instance()->enableAction(ActionManager::DetailTranslate, true);
                    }
                    if (!TTSisWorking && VTextSpeechAndTrManager::getTextToSpeechEnable()) {
                        ActionManager::Instance()->enableAction(ActionManager::DetailText2Speech, true);
                    }
                }
            } else {
                ActionManager::Instance()->enableAction(ActionManager::DetailVoiceSave, true);
                if (isAISrvAvailable && !stateInterface->isVoice2Text() && m_currentBlock->blockText.isEmpty()) {
                    ActionManager::Instance()->enableAction(ActionManager::DetailVoice2Text, true);
                }
            }
        }

        if (canCutDel) {
            ActionManager::Instance()->enableAction(ActionManager::DetailCut, canCutDel);
            ActionManager::Instance()->enableAction(ActionManager::DetailDelete, canCutDel);
        }

        m_noteDetailContextMenu->popup(QCursor::pos());
    }
}

VNoteBlock *JsContent::getCurrentBlock()
{
    return  m_currentBlock;
}

void JsContent::textChange(const QString &id)
{
    m_textChangeMutex.lock();
    if (!id.isEmpty() && !m_textsChange.contains(id)) {
        m_textsChange.push_back(id);
    }
    m_textChangeMutex.unlock();
}

void JsContent::updateNote(QWebEnginePage *page)
{
    m_textChangeMutex.lock();
    if (!m_textsChange.isEmpty() && m_notedata) {
        QEventLoop loop;
        connect(this, &JsContent::updateNoteFinsh, &loop, &QEventLoop::quit);
        for (auto it : m_notedata->datas.dataConstRef()) {
            if (it->getType() == VNoteBlock::Text) {
                for (auto it1 : m_textsChange) {
                    if (it->blockid == it1) {
                        page->runJavaScript(QString("textResult(%1)").arg(it->blockid), [ = ](const QVariant & result) {
                            it->blockText = result.toString();
                            m_textChangeMutex.lock();
                            m_textsChange.removeOne(it->blockid);
                            bool needUpdate = m_textsChange.isEmpty();
                            m_textChangeMutex.unlock();
                            if (needUpdate) {
                                updateNote(m_notedata);
                                emit updateNoteFinsh();
                            }
                        });
                    }
                }
            }
        }
        m_textChangeMutex.unlock();
        loop.exec();
    }else {
        m_textChangeMutex.unlock();
    }
}

void JsContent::updateNote(VNoteItem* notedata)
{
    VNoteItem* tmpData = nullptr == notedata ? m_notedata : notedata;
    if(tmpData){
        VNoteItemOper noteOps(tmpData);
        if (!noteOps.updateNote()) {
            qInfo() << "Save note error";
        }
        qInfo() << "update success";
    }
}

JsContent *JsContent::instance()
{
    if (jsInstance == nullptr) {
        jsInstance = new JsContent;
    }
    return jsInstance;
}
