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
#include "ut_vnoteitem.h"

#define protected public
#include "vnoteitem.h"
#undef protected

ut_vnoteitem_test::ut_vnoteitem_test()
{

}

TEST_F(ut_vnoteitem_test, isValid)
{
    VNoteItem vnoteitem;
    vnoteitem.noteId = 2;
    vnoteitem.folderId = 2;
    vnoteitem.isValid();
}

TEST_F(ut_vnoteitem_test, delNoteData)
{
    VNoteItem vnoteitem;
    vnoteitem.delNoteData();
}

TEST_F(ut_vnoteitem_test, search)
{
    VNoteItem vnoteitem;
    vnoteitem.noteTitle = "test";
    QString tmpstr = "test";
    vnoteitem.search(tmpstr);
}

TEST_F(ut_vnoteitem_test, folder)
{
    VNoteItem vnoteitem;
    vnoteitem.folder();
}

TEST_F(ut_vnoteitem_test, qdebugtest)
{
    VNoteItem vnoteitem;
    vnoteitem.noteId = 0;
    vnoteitem.folderId = 1;
    vnoteitem.noteTitle = "test";
    vnoteitem.createTime = QDateTime::currentDateTime();
    vnoteitem.modifyTime = QDateTime::currentDateTime();
    vnoteitem.deleteTime = QDateTime::currentDateTime();
    qDebug() << "" <<vnoteitem;
}

TEST_F(ut_vnoteitem_test, metaDataRef)
{
    VNoteItem vnoteitem;
    vnoteitem.metaDataRef();
}

TEST_F(ut_vnoteitem_test, metaDataConstRef)
{
    VNoteItem vnoteitem;
    vnoteitem.metaDataConstRef();
}

TEST_F(ut_vnoteitem_test, newBlock)
{
    VNoteItem vnoteitem;
    VNoteBlock *ptrBlock = nullptr;
    ptrBlock = vnoteitem.newBlock(1);
    ASSERT_EQ(ptrBlock->blockType, VNoteBlock::Text);

    ptrBlock = vnoteitem.newBlock(2);
    ASSERT_EQ(ptrBlock->blockType, VNoteBlock::Voice);
}

TEST_F(ut_vnoteitem_test, addBlock)
{
    VNoteItem vnoteitem;
    VNoteBlock *ptrBlock = vnoteitem.newBlock(2);
    VNoteBlock *ptrBlock1 = vnoteitem.newBlock(2);
    vnoteitem.addBlock(ptrBlock);
    vnoteitem.addBlock(ptrBlock, ptrBlock1);
    vnoteitem.delBlock(ptrBlock);
    ASSERT_TRUE(vnoteitem.haveVoice());
    ASSERT_FALSE(vnoteitem.haveText());
    ASSERT_GE(vnoteitem.voiceCount(), 0);
}

TEST_F(ut_vnoteitem_test, setMetadata)
{
    VNoteItem vnoteitem;
    QString tmpstr = "{\"dataCount\":3,\"noteDatas\":[{\"text\":\"wafwaefawffvbdafvadfasdf\",\"type\":1},{\"createTime\":\"2020-08-25 14:39:21.473\",\"state\":false,\"text\":\"\",\"title\":\"Voice2\",\"type\":2,\"voicePath\":\"/usr/share/music/bensound-sunny.mp3\",\"voiceSize\":3630},{\"text\":\"wafwaefawffvbdafvadfasdf\",\"type\":1}],\"voiceMaxId\":3}";
    vnoteitem.setMetadata(tmpstr);
}



















