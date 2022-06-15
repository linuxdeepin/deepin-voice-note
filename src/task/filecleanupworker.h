/*
* Copyright (C) 2019 ~ 2019 UnionTech Software Technology Co.,Ltd.
*
* Author:     leilong <leilong@uniontech.com>
*
* Maintainer: leilong <leilong@uniontech.com>
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
#ifndef FILECLEANUPWORKER_H
#define FILECLEANUPWORKER_H

#include "vntask.h"
#include "datatypedef.h"

#include <QSet>

/**
 * @brief The FileCleanupWorker class
 * 负责清理无用图片和语音的线程类，防止产生大量的内存冗余
 */
class FileCleanupWorker : public VNTask
{
    Q_OBJECT
public:
    explicit FileCleanupWorker(VNOTE_ALL_NOTES_MAP *qspAllNotesMap, QObject *parent = nullptr);

signals:

public slots:

protected:
    virtual void run() override;

private:
    //清理语音
    void cleanVoice();
    //清理图片
    void cleanPicture();
    //获取项目下用户所有的语音完整路径
    void fillVoiceSet();
    //获取项目下用户所有的图片完整路径
    void fillPictureSet();
    //遍历所有的笔记
    bool scanAllNotes();
    //移除语音集合中的指定路径
    void removeVoicePathBySet(const QString &path);
    //移除图片集合中的指定路径
    void removePicturePathBySet(const QString &path);
    //遍历笔记中的语音路径
    void scanVoiceByHtml(const QString &htmlCode);
    //遍历笔记中的图片路径
    void scanPictureByHtml(const QString &htmlCode);
    //遍历5.9及以前版本笔记中的语音路径
    void scanVoiceByBlocks(const VNOTE_DATAS &datas);

private:
    VNOTE_ALL_NOTES_MAP *m_qspAllNotesMap {nullptr}; //所有笔记数据
    QSet<QString> m_pictureSet; //图片路径集合
    QSet<QString> m_voiceSet; //语音路径集合
};

#endif // FILECLEANUPWORKER_H
