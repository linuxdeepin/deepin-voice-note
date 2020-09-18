/*
* Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
*
* Author:     zhangteng <zhangteng@uniontech.com>
* Maintainer: zhangteng <zhangteng@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ut_metadataparser.h"

#define protected public
#include "metadataparser.h"
#undef protected

#include "vnoteitem.h"

ut_metadataparser_test::ut_metadataparser_test()
{

}

TEST_F(ut_metadataparser_test, parse)
{
    MetaDataParser metadataparser;
    QVariant metadata = "{\"dataCount\":1,\"noteDatas\":[{\"text\":\"sdfgsgssrgstg\",\"type\":1}],\"voiceMaxId\":0}";
    VNoteItem *noteData = new VNoteItem();
    ASSERT_TRUE(noteData != nullptr);
    metadataparser.parse(metadata, noteData);
    metadata = "{\"dataCount\":5,\"noteDatas\":[{\"text\":\"\",\"type\":1},{\"createTime\":\"2020-08-18 19:21:15.710\",\"state\":false,\"text\":\"\",\"title\":\"语音1\",\"type\":2,\"voicePath\":\"/usr/share/music/bensound-sunny.mp3\",\"voiceSize\":2650},{\"text\":\"142424\",\"type\":1},{\"createTime\":\"2020-08-18 19:23:24.006\",\"state\":false,\"text\":\"\",\"title\":\"语音2\",\"type\":2,\"voicePath\":\"/usr/share/music/bensound-sunny.mp3\",\"voiceSize\":3860},{\"text\":\"\",\"type\":1}],\"voiceMaxId\":2}";
    metadataparser.parse(metadata, noteData);
    metadataparser.makeMetaData(noteData, metadata);
}
