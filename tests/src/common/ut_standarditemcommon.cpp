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

#include "ut_standarditemcommon.h"
#include "standarditemcommon.h"

UT_StandardItemCommon::UT_StandardItemCommon()
{
}

void UT_StandardItemCommon::SetUp()
{
    m_standarditemcommon = new StandardItemCommon();
}

void UT_StandardItemCommon::TearDown()
{
    delete m_standarditemcommon;
}

TEST_F(UT_StandardItemCommon, UT_StandardItemCommon_getStandardItemType_001)
{
    QStandardItemModel *pDataModel = new QStandardItemModel();
    QStandardItem *pItem = pDataModel->item(0);
    if (pItem == nullptr) {
        pItem = m_standarditemcommon->createStandardItem(nullptr, StandardItemCommon::NOTEPADROOT);
        pDataModel->insertRow(0, pItem);
    }

    QModelIndex index = pDataModel->index(0, 0);
    int ret = m_standarditemcommon->getStandardItemType(index);
    EXPECT_EQ(ret, StandardItemCommon::StandardItemType::NOTEPADROOT) << "getStandardItemType, index(0, 0)";
    EXPECT_FALSE(m_standarditemcommon->getStandardItemData(index)) << "getStandardItemData, index(0, 0)";

    index = pDataModel->index(0, 1);
    ret = m_standarditemcommon->getStandardItemType(index);
    EXPECT_EQ(ret, StandardItemCommon::StandardItemType::Invalid) << "getStandardItemType, index(0, 0)";
    EXPECT_FALSE(m_standarditemcommon->getStandardItemData(index)) << "getStandardItemData, index(0, 1)";
    delete pDataModel;
}
