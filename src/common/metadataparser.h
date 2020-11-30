/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     V4fr3e <V4fr3e@deepin.io>
*
* Maintainer: V4fr3e <liujinli@uniontech.com>
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
#ifndef METADATAPARSER_H
#define METADATAPARSER_H

#include "globaldef.h"
#include "common/datatypedef.h"

#include <QString>
#include <QMap>
#include <QVariant>

/*
Eg: meta-data format
    //------------------------------------------------------------------------
    json-format:

    {
        "dataCount": 2,
        "voiceMaxId":1,
        "noteDatas": [
            {
                "text": "I Love China.",
                "type": 1
            },
            {
                "createTime": "2020-02-27 17:56:36.071",
                "state": false,
                "text": "",
                "title": "语音2",
                "type": 2,
                "voicePath": "/home/voice/1.mp3",
                "voiceSize": 15555
            }
        ]
    }
 */

class MetaDataParser
{
public:
    MetaDataParser();
    enum {
        NDataCount,
        NVoiceMaxId,
        NDatas,
        NDataType,
        NText,
        NTitle,
        NState,
        NVoicePath,
        NVoiceSize,
        NCreateTime,
        NHtmlCode,
        NFormatSize,
    };
    //源数据解析
    void jsonParse(const QVariant &metaData, VNoteBlock *blockData /*out*/);
    void jsonParse(QVariant &metaData, VNoteItem *noteData /*out*/);
    //源数据生成
    void jsonMakeMetadata(VNoteItem *noteData, QVariant &metaData /*out*/);
    void jsonMakeMetadata(VNoteBlock *blockData, QVariant &metaData /*out*/);
protected:
    QMap<int, QString> m_jsonNodeNameMap = {
        {NDataCount, "dataCount"},
        {NVoiceMaxId, "voiceMaxId"},
        {NDatas, "noteDatas"},
        {NDataType, "type"},
        {NText, "text"},
        {NTitle, "title"},
        {NState, "state"}, // State: 1 for audio to text done, 0 for have not done
        {NVoicePath, "voicePath"},
        {NVoiceSize, "voiceSize"},
        {NCreateTime, "createTime"},
        {NHtmlCode, "htmlCode"},
        {NFormatSize, "transSize"},
    };
};

#endif // METADATAPARSER_H
