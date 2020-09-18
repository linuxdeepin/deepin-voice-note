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

#include "ut_datatypedef.h"

#define protected public
#include "datatypedef.h"
#undef protected

#include "vnoteforlder.h"
#include "vnoteitem.h"
#include "globaldef.h"

#include <DLog>

ut_datatypedef_test::ut_datatypedef_test()
{

}

TEST_F(ut_datatypedef_test, dataConstRef)
{
    VNOTE_DATAS vnote_datas;
    VNOTE_DATA_VECTOR vnote_data_vector;
    ASSERT_TRUE(vnote_data_vector.size() == 0);

    VNoteBlock *ptrBlock = nullptr;
    vnote_datas.datas.push_back(ptrBlock);
    vnote_data_vector = vnote_datas.dataConstRef();
    ASSERT_FALSE(vnote_data_vector.size() == 0);
}

TEST_F(ut_datatypedef_test, newBlock)
{
    VNOTE_DATAS vnote_datas;
    VNoteBlock *ptrBlock = nullptr;
    ptrBlock = vnote_datas.newBlock(1);
    ASSERT_EQ(ptrBlock->blockType, VNoteBlock::Text);

    ptrBlock = vnote_datas.newBlock(2);
    ASSERT_EQ(ptrBlock->blockType, VNoteBlock::Voice);
}

TEST_F(ut_datatypedef_test, addBlock)
{
    VNOTE_DATAS vnote_datas;
    VNoteBlock *ptrBlock = vnote_datas.newBlock(1);
    vnote_datas.addBlock(ptrBlock);
    ASSERT_NE(vnote_datas.datas.size(), 0);
}

TEST_F(ut_datatypedef_test, addBlock1)
{
    VNOTE_DATAS vnote_datas;
    VNoteBlock *ptrBlock = vnote_datas.newBlock(1);
    VNoteBlock *ptrBlock1 = vnote_datas.newBlock(2);
    vnote_datas.addBlock(ptrBlock);
    vnote_datas.addBlock(ptrBlock, ptrBlock1);
    ASSERT_NE(vnote_datas.datas.size(), 0);

    VNoteBlock *ptrBlock2 = vnote_datas.newBlock(1);
    vnote_datas.addBlock(ptrBlock2, ptrBlock2);
    ASSERT_NE(vnote_datas.datas.size(), 0);

    ptrBlock2 = nullptr;
    VNoteBlock *ptrBlock3 = vnote_datas.newBlock(2);
    vnote_datas.addBlock(ptrBlock2, ptrBlock3);
    ASSERT_NE(vnote_datas.datas.size(), 0);
}

TEST_F(ut_datatypedef_test, delBlock)
{
    VNOTE_DATAS vnote_datas;
    VNoteBlock *ptrBlock = vnote_datas.newBlock(1);
    VNoteBlock *ptrBlock1 = vnote_datas.newBlock(2);
    vnote_datas.addBlock(ptrBlock);
    vnote_datas.addBlock(ptrBlock, ptrBlock1);
    ASSERT_NE(vnote_datas.datas.size(), 0);

    vnote_datas.delBlock(ptrBlock);
    ASSERT_NE(vnote_datas.datas.size(), 0);
}

TEST_F(ut_datatypedef_test, classifyAddBlk)
{
    VNOTE_DATAS vnote_datas;
    VNoteBlock *ptrBlock = vnote_datas.newBlock(2);
    ptrBlock->blockType = VNoteBlock::InValid;
    vnote_datas.classifyAddBlk(ptrBlock);
    ASSERT_EQ(vnote_datas.datas.size(), 0);
}

TEST_F(ut_datatypedef_test, classifyDelBlk)
{
    VNOTE_DATAS vnote_datas;
    VNoteBlock *ptrBlock = vnote_datas.newBlock(2);
    vnote_datas.classifyAddBlk(ptrBlock);
    vnote_datas.classifyDelBlk(ptrBlock);

    ptrBlock->blockType = VNoteBlock::InValid;
    vnote_datas.classifyAddBlk(ptrBlock);
    vnote_datas.classifyDelBlk(ptrBlock);
}

TEST_F(ut_datatypedef_test, isValid)
{
    VDataSafer vdatasafer;
    ASSERT_FALSE(vdatasafer.isValid());
}

TEST_F(ut_datatypedef_test, setSaferType)
{
    VDataSafer vdatasafer;
    vdatasafer.setSaferType(vdatasafer.Unsafe);
    ASSERT_EQ(vdatasafer.saferType, vdatasafer.Unsafe);
}

TEST_F(ut_datatypedef_test, qdebugout)
{
    VDataSafer vdatasafer;
    vdatasafer.path = "test";
    vdatasafer.meta_data = "test";
    vdatasafer.createTime = QDateTime::currentDateTime();
    qDebug() << "" << vdatasafer;
}

TEST_F(ut_datatypedef_test, TEST_VNOTE_FOLDERS_MAP)
{
    VNOTE_FOLDERS_MAP vnote_floders_map;
    VNoteFolder *vnotefloder = nullptr;
    vnote_floders_map.folders.insert(1, vnotefloder);
    vnote_floders_map.autoRelease = true;
}

TEST_F(ut_datatypedef_test, TEST_VNOTE_ITEMS_MAP)
{
    VNOTE_ITEMS_MAP vnote_items_map;
    VNoteItem *vnoteitem = nullptr;
    vnote_items_map.folderNotes.insert(1, vnoteitem);
    vnote_items_map.autoRelease = true;
}

TEST_F(ut_datatypedef_test, TEST_VNOTE_ALL_NOTES_MAP)
{
    VNOTE_ALL_NOTES_MAP vnote_all_notes_map;
    VNOTE_ITEMS_MAP *vnote_items_map = nullptr;
    vnote_all_notes_map.notes.insert(1, vnote_items_map);
    vnote_all_notes_map.autoRelease = true;
}
















