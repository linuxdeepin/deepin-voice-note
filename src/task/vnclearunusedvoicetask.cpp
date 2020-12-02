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
#include "vnclearunusedvoicetask.h"
#include "common/vnotedatamanager.h"
#include "common/vnoteforlder.h"
#include "common/vnoteitem.h"
#include "db/vnoteitemoper.h"

#include <QStandardPaths>
#include <QDir>
#include <QDebug>
VNClearUnusedVoiceTask::VNClearUnusedVoiceTask(QObject *parent)
    :VNTask (parent)
{

}

void VNClearUnusedVoiceTask::run()
{
    QString voicePath =  QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    voicePath += "/voicenote/";
    QDir dir(voicePath);
    if(dir.exists()){
        dir.setFilter(QDir::Files | QDir::NoSymLinks);
        int count = static_cast<int>(dir.count());
        VNOTE_FOLDERS_MAP *folders = VNoteDataManager::instance()->getNoteFolders();
        QVector<int> delVoices;
        if(folders){
            VNoteItemOper noteOper;
            folders->lock.lockForRead();
            for(int i=0; i<count; i++){
                QString fullPath = voicePath + dir[i];
                bool delFlag = true;
                for(auto it : folders->folders){
                    VNOTE_ITEMS_MAP *notes = noteOper.getFolderNotes(it->id);
                    if(notes){
                        notes->lock.lockForRead();
                        for(auto it1 : notes->folderNotes){
                            if(it1->htmlCode.isEmpty() || it1->htmlCode.contains(fullPath)){
                                delFlag = false;
                                break;
                            }
                        }
                        notes->lock.unlock();
                    }
                    if(!delFlag){
                        break;
                    }
                }
                if(delFlag){
                    delVoices.push_back(i);
                }
            }
            folders->lock.unlock();
            for(auto it : delVoices){
                dir.remove(dir[it]);
            }
        }
    }
}
