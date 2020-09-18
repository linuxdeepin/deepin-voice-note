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

ut_standarditemcommon_test::ut_standarditemcommon_test()
{

}

void ut_standarditemcommon_test::SetUp()
{
    m_standarditemcommon = new StandardItemCommon();
}

void ut_standarditemcommon_test::TearDown()
{
    delete m_standarditemcommon;
}

TEST_F(ut_standarditemcommon_test, getStandardItemType)
{
    QStandardItemModel *pDataModel = new QStandardItemModel();
    QStandardItem *pItem = pDataModel->item(0);
    if (pItem == nullptr) {
        pItem = m_standarditemcommon->createStandardItem(nullptr, StandardItemCommon::NOTEPADROOT);
        pDataModel->insertRow(0, pItem);
    }

    QModelIndex index = pDataModel->index(0,0);
    m_standarditemcommon->getStandardItemType(index);
    m_standarditemcommon->getStandardItemData(index);

    index = pDataModel->index(0,1);
    m_standarditemcommon->getStandardItemType(index);
    m_standarditemcommon->getStandardItemData(index);
}
