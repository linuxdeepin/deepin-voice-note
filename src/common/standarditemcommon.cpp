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
#include "standarditemcommon.h"

/**
 * @brief StandardItemCommon::StandardItemCommon
 */
StandardItemCommon::StandardItemCommon()
{
}

/**
 * @brief StandardItemCommon::createStandardItem
 * @param data 绑定的数据
 * @param type
 * @return 生成数据项
 */
QStandardItem *StandardItemCommon::createStandardItem(void *data, StandardItemType type)
{
    QStandardItem *pItem = new QStandardItem;
    pItem->setData(QVariant::fromValue(type), Qt::UserRole + 1);
    pItem->setData(QVariant::fromValue(data), Qt::UserRole + 2);
    return pItem;
}

/**
 * @brief StandardItemCommon::getStandardItemType
 * @param index 索引
 * @return 数据项类型
 */
StandardItemCommon::StandardItemType StandardItemCommon::getStandardItemType(const QModelIndex &index)
{
    StandardItemType type = Invalid;
    if (index.isValid()) {
        QVariant var = index.data(Qt::UserRole + 1);
        if (var.isValid()) {
            return var.value<StandardItemType>();
        }
    }
    return type;
}

/**
 * @brief StandardItemCommon::getStandardItemData
 * @param index
 * @return 数据项绑定的数据
 */
void *StandardItemCommon::getStandardItemData(const QModelIndex &index)
{
    if (index.isValid()) {
        QVariant var = index.data(Qt::UserRole + 2);
        if (var.isValid()) {
            return var.value<void *>();
        }
    }
    return nullptr;
}
